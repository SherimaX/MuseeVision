// ProcessJob.cs: one launched command inside its own Windows job object.
//
// unreal.ps1's worker compiles this with Add-Type. Everything the command starts (Unreal's ShaderCompileWorkers,
// UnrealBuildTool's compilers) stays inside the job, so the worker can wait for all of it, and if the worker dies
// Windows closes the job and ends the command with it (JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE). The job allows breakaway
// (JOB_OBJECT_LIMIT_BREAKAWAY_OK), so the shared zenserver and trace server, which Unreal starts with
// CREATE_BREAKAWAY_FROM_JOB, are never captured. Any shared service that still lands inside the job is never
// terminated by this class; KeepSharedServices clears kill-on-close before the job is closed.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace MuseeWorkshop
{
    public sealed class ProcessJob : IDisposable
    {
        const uint LimitKillOnJobClose = 0x00002000;
        const uint LimitBreakawayOk = 0x00000800;
        const int InfoBasicProcessIdList = 3;
        const int InfoExtendedLimits = 9;
        const uint CreateSuspended = 0x00000004;
        const uint CreateNoWindow = 0x08000000;
        const uint StartfUseStdHandles = 0x00000100;
        const uint StartfUseShowWindow = 0x00000001;
        const uint GenericRead = 0x80000000;
        const uint GenericWrite = 0x40000000;
        const uint ShareReadWrite = 0x00000003;
        const uint CreateAlways = 2;
        const uint OpenExisting = 3;
        const uint ProcessTerminate = 0x0001;
        const uint WaitObject0 = 0;

        IntPtr job;
        IntPtr process;
        public int ProcessId { get; private set; }

        public ProcessJob()
        {
            job = CreateJobObject(IntPtr.Zero, null);
            if (job == IntPtr.Zero) { throw new Win32Exception(); }
            SetLimits(LimitKillOnJobClose | LimitBreakawayOk);
        }

        // Starts the command line suspended, puts it in the job, then lets it run, so nothing it starts escapes.
        // Standard output and standard error both go to the log file; standard input reads nothing.
        public void Start(string commandLine, string workingDirectory, string logPath)
        {
            var inherit = new SecurityAttributes();
            inherit.Length = Marshal.SizeOf(inherit);
            inherit.InheritHandle = true;
            IntPtr log = CreateFile(logPath, GenericWrite, ShareReadWrite, ref inherit, CreateAlways, 0, IntPtr.Zero);
            if (log == new IntPtr(-1)) { throw new Win32Exception(); }
            IntPtr none = CreateFile("NUL", GenericRead, ShareReadWrite, ref inherit, OpenExisting, 0, IntPtr.Zero);
            try
            {
                var startup = new StartupInfo();
                startup.Size = Marshal.SizeOf(startup);
                startup.Flags = StartfUseStdHandles | StartfUseShowWindow;
                startup.ShowWindow = 0;
                startup.StdInput = none;
                startup.StdOutput = log;
                startup.StdError = log;
                ProcessInformation info;
                var line = new StringBuilder(commandLine);
                string folder = string.IsNullOrEmpty(workingDirectory) ? null : workingDirectory;
                if (!CreateProcess(null, line, IntPtr.Zero, IntPtr.Zero, true, CreateSuspended | CreateNoWindow,
                        IntPtr.Zero, folder, ref startup, out info))
                {
                    throw new Win32Exception();
                }
                if (!AssignProcessToJobObject(job, info.Process))
                {
                    int error = Marshal.GetLastWin32Error();
                    TerminateProcess(info.Process, 1);
                    throw new Win32Exception(error);
                }
                ResumeThread(info.Thread);
                CloseHandle(info.Thread);
                process = info.Process;
                ProcessId = info.ProcessId;
            }
            finally
            {
                CloseHandle(log);
                CloseHandle(none);
            }
        }

        public bool WaitForMain(int milliseconds)
        {
            return WaitForSingleObject(process, (uint)milliseconds) == WaitObject0;
        }

        public int MainExitCode
        {
            get { uint code; GetExitCodeProcess(process, out code); return unchecked((int)code); }
        }

        // The ids of every process still inside the job.
        public int[] ProcessIds()
        {
            int capacity = 4096;
            int size = 8 + IntPtr.Size * capacity;
            IntPtr buffer = Marshal.AllocHGlobal(size);
            try
            {
                int written;
                if (!QueryInformationJobObject(job, InfoBasicProcessIdList, buffer, size, out written))
                {
                    throw new Win32Exception();
                }
                int count = Marshal.ReadInt32(buffer, 4);
                var ids = new int[count];
                for (int i = 0; i < count; i++) { ids[i] = (int)Marshal.ReadIntPtr(buffer, 8 + i * IntPtr.Size).ToInt64(); }
                return ids;
            }
            finally { Marshal.FreeHGlobal(buffer); }
        }

        // The processes inside the job other than the named shared services, as "name (pid)" strings.
        public string[] Describe(string[] sharedServices)
        {
            var found = new List<string>();
            foreach (int id in ProcessIds())
            {
                string name = NameOf(id);
                if (name != null && !IsShared(name, sharedServices)) { found.Add(name + " (" + id + ")"); }
            }
            return found.ToArray();
        }

        // Ends every process in the job except the named shared services. Returns how many were ended.
        public int TerminateExcept(string[] sharedServices)
        {
            int ended = 0;
            foreach (int id in ProcessIds())
            {
                string name = NameOf(id);
                if (name == null || IsShared(name, sharedServices)) { continue; }
                IntPtr handle = OpenProcess(ProcessTerminate, false, id);
                if (handle == IntPtr.Zero) { continue; }
                if (TerminateProcess(handle, 1)) { ended++; }
                CloseHandle(handle);
            }
            return ended;
        }

        // True when a shared service landed inside the job (its breakaway was refused by an outer job).
        public bool HoldsShared(string[] sharedServices)
        {
            foreach (int id in ProcessIds())
            {
                string name = NameOf(id);
                if (name != null && IsShared(name, sharedServices)) { return true; }
            }
            return false;
        }

        // Lets shared services outlive the job: closing the job no longer ends what is inside it.
        public void KeepSharedServices()
        {
            SetLimits(LimitBreakawayOk);
        }

        public void Dispose()
        {
            if (process != IntPtr.Zero) { CloseHandle(process); process = IntPtr.Zero; }
            if (job != IntPtr.Zero) { CloseHandle(job); job = IntPtr.Zero; }
        }

        // Physical memory Windows can hand to a new process now (free plus standby), in bytes.
        public static ulong AvailableMemoryBytes()
        {
            var status = new MemoryStatus();
            status.Length = (uint)Marshal.SizeOf(status);
            if (!GlobalMemoryStatusEx(ref status)) { throw new Win32Exception(); }
            return status.AvailablePhysical;
        }

        static string NameOf(int id)
        {
            try { using (var p = Process.GetProcessById(id)) { return p.ProcessName + ".exe"; } }
            catch (ArgumentException) { return null; }
            catch (InvalidOperationException) { return null; }
        }

        static bool IsShared(string name, string[] sharedServices)
        {
            foreach (string service in sharedServices)
            {
                if (string.Equals(name, service, StringComparison.OrdinalIgnoreCase)) { return true; }
            }
            return false;
        }

        void SetLimits(uint flags)
        {
            var limits = new ExtendedLimits();
            limits.Basic.LimitFlags = flags;
            int size = Marshal.SizeOf(limits);
            if (!SetInformationJobObject(job, InfoExtendedLimits, ref limits, size)) { throw new Win32Exception(); }
        }

        [StructLayout(LayoutKind.Sequential)]
        struct SecurityAttributes { public int Length; public IntPtr Descriptor; public bool InheritHandle; }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        struct StartupInfo
        {
            public int Size; public string Reserved; public string Desktop; public string Title;
            public int X; public int Y; public int XSize; public int YSize; public int XCountChars; public int YCountChars;
            public int ConsoleAttribute; public uint Flags; public short ShowWindow; public short Reserved2;
            public IntPtr Reserved3; public IntPtr StdInput; public IntPtr StdOutput; public IntPtr StdError;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct ProcessInformation { public IntPtr Process; public IntPtr Thread; public int ProcessId; public int ThreadId; }

        [StructLayout(LayoutKind.Sequential)]
        struct BasicLimits
        {
            public long PerProcessUserTimeLimit; public long PerJobUserTimeLimit; public uint LimitFlags;
            public UIntPtr MinimumWorkingSetSize; public UIntPtr MaximumWorkingSetSize; public uint ActiveProcessLimit;
            public UIntPtr Affinity; public uint PriorityClass; public uint SchedulingClass;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct IoCounters
        {
            public ulong ReadOperationCount; public ulong WriteOperationCount; public ulong OtherOperationCount;
            public ulong ReadTransferCount; public ulong WriteTransferCount; public ulong OtherTransferCount;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct ExtendedLimits
        {
            public BasicLimits Basic; public IoCounters Io; public UIntPtr ProcessMemoryLimit; public UIntPtr JobMemoryLimit;
            public UIntPtr PeakProcessMemoryUsed; public UIntPtr PeakJobMemoryUsed;
        }

        [StructLayout(LayoutKind.Sequential)]
        struct MemoryStatus
        {
            public uint Length; public uint MemoryLoad; public ulong TotalPhysical; public ulong AvailablePhysical;
            public ulong TotalPageFile; public ulong AvailablePageFile; public ulong TotalVirtual; public ulong AvailableVirtual;
            public ulong AvailableExtendedVirtual;
        }

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern IntPtr CreateJobObject(IntPtr attributes, string name);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool SetInformationJobObject(IntPtr job, int infoClass, ref ExtendedLimits info, int size);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool QueryInformationJobObject(IntPtr job, int infoClass, IntPtr info, int size, out int written);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool AssignProcessToJobObject(IntPtr job, IntPtr process);
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern bool CreateProcess(string application, StringBuilder commandLine, IntPtr processAttributes,
            IntPtr threadAttributes, bool inheritHandles, uint flags, IntPtr environment, string currentDirectory,
            ref StartupInfo startup, out ProcessInformation info);
        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        static extern IntPtr CreateFile(string name, uint access, uint share, ref SecurityAttributes security,
            uint disposition, uint attributes, IntPtr template);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint ResumeThread(IntPtr thread);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr handle);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint WaitForSingleObject(IntPtr handle, uint milliseconds);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool GetExitCodeProcess(IntPtr process, out uint code);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr OpenProcess(uint access, bool inherit, int id);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool TerminateProcess(IntPtr process, uint code);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool GlobalMemoryStatusEx(ref MemoryStatus status);
    }
}
