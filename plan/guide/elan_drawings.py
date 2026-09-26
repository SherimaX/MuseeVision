"""The guide's new drawings, computed to scale from the museum's numbers (metres).

guide.html holds their output inline. When an Élan number changes (the Sphere, the drum, the
opening, the car, the ride), change it below, print the drawing and paste it over the old <svg>
in guide.html (each is marked by a comment naming its function):

    python elan_drawings.py elan_section > section.svg
    python elan_drawings.py ride_chart   > ride.svg

Drawings: keyplan, elan_floors, atrium_elevation, atrium_half_section, elan_section,
spheres_compare, ride_chart, masterplan_level2, pc_keys. Labels use the classes in guide.css.
"""
import math, random

INK = "#1E1C19"; TEAL = "#2C6E73"; GLASS = "#7CC3C5"; GILT = "#7E5C25"; GILT_L = "#C9A266"
NIGHT = "#1E2A33"; CARD = "#FBF8F2"; TRAV = "#E4DBCB"; MUTED = "#5E574D"; RIB = "#2A2723"
MIST = "#EAF1F0"; MIST_EDGE = "#9FBFC0"

# Élan (metres): Atrium R 14, wall 0.8, base 6, drum to 22; Sphere R 14, centre 22.
R = 14.0; WALL = 0.8; BASE = 6.0; C = 22.0
RING_R = 3.0
RING_Z = C - math.sqrt(R * R - RING_R * RING_R)      # 8.325
POLE = C - R                                          # 8
EYE = 1.6
TOP_FLOOR = C - EYE                                   # 20.4
HOME = RING_Z + 0.3                                   # 8.63
CAR_R = 2.2; CAR_H = 2.6; SCREEN_R = 2.3; SCREEN_H = 2.7
SQ_FLOOR = -9.0; SQ_CEIL = -1.5
SPEED = 1.5; ACC = 0.75


def f(v):
    return f"{v:.1f}".rstrip("0").rstrip(".") if abs(v - round(v)) > 1e-9 else str(int(round(v)))


def stars(rng, n, inside, r=1.2, fill="#F1EBDF", op=None):
    out = []
    while len(out) < n:
        x, y = inside[0](rng)
        if inside[1](x, y):
            o = f";opacity:{op}" if op else ""
            out.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r * rng.choice([0.7, 0.9, 1.0, 1.3]):.2f}" style="fill:{fill}{o}"/>')
    return "".join(out)


# ---------------------------------------------------------------- key plan (metres)

def keyplan(highlight=None, theme="dark", numbers=False, labels=False):
    """Schematic plan of the museum at true proportions; viewBox in metres."""
    if theme == "dark":
        st, gn, gs, axis, hl, hls, bg, eye = "#6E6555", "#2E362C", "#3A2F2E", "#C9A266", "#C9A266", "#E6CD95", "none", "#7CC3C5"
    else:
        st, gn, gs, axis, hl, hls, bg, eye = "#A39A8A", "#DDE4D5", "#EEDCD8", "#C9A266", "#C9A266", "#E6CD95", "none", "#7CC3C5"
    sw = 0.55
    def fill(k):
        return hl if highlight == k else "none"
    def stroke(k):
        return hls if highlight == k else st
    p = []
    # gardens either side of the hall
    p.append(f'<rect x="10.8" y="-14" width="28.4" height="9.5" style="fill:{gn}"/>')
    p.append(f'<rect x="10.8" y="4.5" width="28.4" height="9.5" style="fill:{gs}"/>')
    # Salon: five bays 12 m, 14 m wide, from x -13.4 west
    p.append(f'<rect x="-73.4" y="-7" width="60" height="14" style="fill:{fill("salon")};stroke:{stroke("salon")};stroke-width:{sw}"/>')
    for i in range(1, 5):
        x = -13.4 - 12 * i
        p.append(f'<line x1="{x}" y1="-7" x2="{x}" y2="7" style="stroke:{stroke("salon")};stroke-width:{sw * 0.6}"/>')
    # Manet cabinet on Bay 1's north side
    p.append(f'<rect x="-22.4" y="-15.4" width="8.4" height="8.4" style="fill:{fill("salon")};stroke:{stroke("salon")};stroke-width:{sw}"/>')
    # Nymphéas oval
    p.append(f'<ellipse cx="-85.4" cy="0" rx="12" ry="8.6" style="fill:{fill("oval")};stroke:{stroke("oval") if highlight != "salon" else st};stroke-width:{sw}"/>')
    # Rotunda
    p.append(f'<circle cx="0" cy="0" r="10.8" style="fill:{fill("rotunda")};stroke:{stroke("rotunda")};stroke-width:{sw}"/>')
    # Sculpture Hall (north)
    p.append(f'<rect x="-9.2" y="-31" width="18.4" height="18.4" style="fill:{fill("sculpture")};stroke:{stroke("sculpture")};stroke-width:{sw}"/>')
    # Chinese Wing (south)
    p.append(f'<rect x="-10" y="13" width="20" height="20.4" style="fill:{fill("chinese")};stroke:{stroke("chinese")};stroke-width:{sw}"/>')
    # Hall of Light
    p.append(f'<rect x="10.8" y="-4.5" width="28.4" height="9" style="fill:{fill("hall")};stroke:{stroke("hall")};stroke-width:{sw}"/>')
    # Atrium
    p.append(f'<circle cx="54" cy="0" r="14.8" style="fill:{fill("elan")};stroke:{stroke("elan")};stroke-width:{sw}"/>')
    # axes
    p.append(f'<line x1="-94" y1="0" x2="52" y2="0" style="stroke:{axis};stroke-width:0.32;stroke-dasharray:1 1.2"/>')
    p.append(f'<line x1="0" y1="-26" x2="0" y2="27" style="stroke:{axis};stroke-width:0.32;stroke-dasharray:1 1.2"/>')
    p.append(f'<circle cx="0" cy="0" r="{1.2 if highlight != "rotunda" else 1.3}" style="fill:{axis if highlight != "rotunda" else "#F4E3BC"}"/>')
    p.append(f'<circle cx="54" cy="0" r="2.2" style="fill:none;stroke:{eye};stroke-width:0.55"/>')
    if numbers:
        for n, (x, y) in {1: (0, 0), 2: (-43.4, 0), 3: (0, -24), 4: (0, 26), 5: (25, 0), 6: (54, 0)}.items():
            p.append(f'<circle cx="{x}" cy="{y}" r="3.6" style="fill:{INK};stroke:{GILT_L};stroke-width:0.55"/>')
            p.append(f'<text x="{x}" y="{y + 1.45}" style="font-family:var(--sans);font-size:4.2px;fill:#FBF8F2;text-anchor:middle">{n}</text>')
    if labels:
        L = [("NYMPHÉAS OVAL", -85.4, 14.6), ("SALON IMPRESSION", -43.4, 12.2), ("ROTUNDA", -12, 15.8, "end"),
             ("SCULPTURE HALL", 11.5, -26.5, "start"), ("CHINESE WING", 11.8, 27.5, "start"),
             ("HALL OF LIGHT", 25, 19.6), ("ÉLAN", 54, 20.4)]
        for t in L:
            anchor = t[3] if len(t) > 3 else "middle"
            p.append(f'<text x="{t[1]}" y="{t[2]}" style="font-family:var(--mono);font-size:2.9px;letter-spacing:0.12em;fill:{MUTED};text-anchor:{anchor}">{t[0]}</text>')
    vb = "-99 -33 170 68" if not labels else "-99 -33 170 68"
    return f'<svg viewBox="{vb}" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="Key plan of the museum">{"".join(p)}</svg>'


# ---------------------------------------------------------------- Élan in three floors (true proportions)

def elan_floors(here="atrium", big=False):
    s = 4.4
    X0, Y0 = 80.0, 170.0
    def Y(z): return Y0 - z * s
    half = R * s
    p = []
    rng = random.Random(7)
    # the Sphere (sky inside); only the upper half shows above the drum
    # drum: travertine base, misty glass to 22 m; the Sphere sits in it like a ball in a cup
    p.append(f'<rect x="{f(X0 - half)}" y="{f(Y(C))}" width="{f(2 * half)}" height="{f((C - BASE) * s)}" style="fill:{MIST}"/>')
    p.append(f'<circle cx="{X0}" cy="{f(Y(C))}" r="{f(half)}" style="fill:{NIGHT}"/>')
    p.append(stars(rng, 16, (lambda r: (X0 + r.uniform(-half, half), r.uniform(Y(C + R), Y(RING_Z))),
                              lambda x, y: (x - X0) ** 2 + (y - Y(C)) ** 2 < (half - 5) ** 2 and y < Y(RING_Z) - 8)))
    # the opening at the south pole, over the car
    p.append(f'<rect x="{f(X0 - RING_R * s)}" y="{f(Y(RING_Z))}" width="{f(2 * RING_R * s)}" height="{f((RING_Z - POLE) * s + 1)}" style="fill:{MIST}"/>')
    p.append(f'<path d="M {f(X0 - half)} {f(Y(C))} L {f(X0 - half)} {f(Y(BASE))} M {f(X0 + half)} {f(Y(C))} L {f(X0 + half)} {f(Y(BASE))}" style="fill:none;stroke:{TEAL};stroke-width:1.6"/>')
    p.append(f'<rect x="{f(X0 - half)}" y="{f(Y(BASE))}" width="{f(2 * half)}" height="{f(BASE * s)}" style="fill:{TRAV};stroke:{INK};stroke-width:1.2"/>')
    # ground and the Square
    p.append(f'<line x1="6" y1="{f(Y0)}" x2="154" y2="{f(Y0)}" style="stroke:{INK};stroke-width:1.5"/>')
    p.append(f'<rect x="{f(X0 - half)}" y="{f(Y(SQ_CEIL))}" width="{f(2 * half)}" height="{f((SQ_CEIL - SQ_FLOOR) * s)}" style="fill:{TRAV};stroke:{INK};stroke-width:1.5"/>')
    # the car's line, and the car where you are
    p.append(f'<line x1="{X0}" y1="{f(Y(SQ_FLOOR) - 2)}" x2="{X0}" y2="{f(Y(POLE))}" style="stroke:{TEAL};stroke-width:2;stroke-dasharray:4 3"/>')
    p.append(f'<line x1="{X0}" y1="{f(Y(POLE))}" x2="{X0}" y2="{f(Y(C))}" style="stroke:{GILT_L};stroke-width:1.6"/>')
    cz = {"atrium": 0, "square": SQ_FLOOR, "sphere": TOP_FLOOR}[here]
    cw = 2 * CAR_R * s
    p.append(f'<rect x="{f(X0 - cw / 2)}" y="{f(Y(cz + CAR_H))}" width="{f(cw)}" height="{f(CAR_H * s)}" style="fill:#BFE0E0;stroke:{TEAL};stroke-width:1.3"/>')
    p.append(f'<circle cx="{X0}" cy="{f(Y(C))}" r="2.2" style="fill:{GLASS}"/>' if here != "sphere" else "")
    # labels
    lab = []
    def block(y, lvl, name, col, cap, lead_y, lead_x):
        lab.append(f'<line x1="{lead_x}" y1="{f(lead_y)}" x2="158" y2="{f(lead_y)}" style="stroke:#D9D0C1;stroke-width:1"/>')
        lab.append(f'<text x="166" y="{f(y)}" class="lbl">{lvl}</text>')
        lab.append(f'<text x="166" y="{f(y + 17)}" class="lbl" style="fill:{col}">{name}</text>')
        lab.append(f'<text x="166" y="{f(y + 34)}" class="cap">{cap}</text>')
    here_txt = lambda k: " · YOU ARE HERE" if here == k else ""
    block(22, "LEVEL 2", "THE SPHERE · ROUND SKY", INK, "The live sky, and new art", 50, 142)
    block(104, "LEVEL 0", "THE ATRIUM" + (" · YOU ARE HERE" if here == "atrium" else " · MISTY GLASS"), TEAL, "New art, and The Starry Night", 124, 146)
    block(184, "LEVEL −1", "THE SQUARE · EARTH", GILT, "New art, below the Atrium", 192, 146)
    return (f'<svg viewBox="0 0 400 236" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="Élan in three floors, at true proportions: '
            f'the Square below ground, the round Atrium, and the Sphere sitting on the Atrium\'s drum like a ball in a cup, joined by the glass car">'
            + "".join(p) + "".join(lab) + "</svg>")


# ---------------------------------------------------------------- The Atrium: the one painting and the round glass car

def atrium_elevation():
    s = 40.0
    G = 226.0
    def Y(z): return G - z * s
    p = []
    p.append(f'<line x1="0" y1="{G}" x2="512" y2="{G}" style="stroke:{INK};stroke-width:1.5"/>')
    cx = 340.0
    # the mast line up to the Sphere, with a few stars
    p.append(f'<line x1="{cx}" y1="{f(Y(SCREEN_H) - 2)}" x2="{cx}" y2="0" style="stroke:{TEAL};stroke-width:1.4;stroke-dasharray:4 3"/>')
    for (x, y) in [(362, 20), (322, 40), (386, 58), (352, 84), (398, 30), (310, 70)]:
        p.append(f'<circle cx="{x}" cy="{y}" r="1.8" style="fill:{GILT_L}"/>')
    # enclosure (landing glass) Ø 4.6 × 2.7 m, its doors open on the west
    ew, eh = 2 * SCREEN_R * s, SCREEN_H * s
    p.append(f'<rect x="{f(cx - ew / 2)}" y="{f(Y(SCREEN_H))}" width="{f(ew)}" height="{f(eh)}" style="fill:#D8ECEC;fill-opacity:0.35;stroke:{TEAL};stroke-width:1.1"/>')
    # car Ø 4.4 × 2.6 m
    cw, ch = 2 * CAR_R * s, CAR_H * s
    x0, x1 = cx - cw / 2, cx + cw / 2
    p.append(f'<rect x="{f(x0)}" y="{f(Y(CAR_H))}" width="{f(cw)}" height="{f(ch)}" style="fill:#BFE0E0;fill-opacity:0.42;stroke:{TEAL};stroke-width:1.4"/>')
    # curvature: vertical highlights where the cylinder turns away
    for deg in (-72, -52, -30, 30, 52, 72):
        xx = cx + CAR_R * s * math.sin(math.radians(deg))
        p.append(f'<line x1="{f(xx)}" y1="{f(Y(CAR_H) + 12)}" x2="{f(xx)}" y2="{f(G - 6)}" style="stroke:#FFFFFF;stroke-width:1.2;opacity:0.55"/>')
    # door opening: half-angle 24°, chord 1.79 m; the curved leaves slid round either side
    half = CAR_R * s * math.sin(math.radians(24))
    p.append(f'<rect x="{f(cx - half)}" y="{f(Y(2.3))}" width="{f(2 * half)}" height="{f(2.3 * s - 5)}" style="fill:{CARD};fill-opacity:0.9;stroke:{TEAL};stroke-width:0.8"/>')
    for sgn in (-1, 1):
        a = cx + sgn * half
        b = cx + sgn * CAR_R * s * math.sin(math.radians(46))
        xa, xb = (b, a) if sgn < 0 else (a, b)
        p.append(f'<rect x="{f(xa)}" y="{f(Y(2.3))}" width="{f(xb - xa)}" height="{f(2.3 * s - 5)}" style="fill:#CFE8E8;fill-opacity:0.55;stroke:{TEAL};stroke-width:0.6"/>')
        ay = Y(2.3) + 14
        tip = cx + sgn * (half + 30)
        p.append(f'<line x1="{f(cx + sgn * (half + 6))}" y1="{f(ay)}" x2="{f(tip)}" y2="{f(ay)}" style="stroke:{TEAL};stroke-width:1"/>')
        p.append(f'<polygon points="{f(tip + sgn * 5)},{f(ay)} {f(tip)},{f(ay - 3)} {f(tip)},{f(ay + 3)}" style="fill:{TEAL}"/>')
    # luminous canopy and the slim bronze rings
    p.append(f'<rect x="{f(x0 + 2)}" y="{f(Y(CAR_H) + 4)}" width="{f(cw - 4)}" height="9" style="fill:#FFF3D6"/>')
    p.append(f'<rect x="{f(x0 + 2)}" y="{f(Y(CAR_H) + 13)}" width="{f(cw - 4)}" height="10" style="fill:#FFF3D6;opacity:0.45"/>')
    p.append(f'<rect x="{f(x0 - 1)}" y="{f(Y(CAR_H))}" width="{f(cw + 2)}" height="4" style="fill:#A07F4A"/>')
    p.append(f'<rect x="{f(x0 - 1)}" y="{f(G - 5)}" width="{f(cw + 2)}" height="5" style="fill:#A07F4A"/>')
    # rail at 1.05 m
    p.append(f'<line x1="{f(x0 + 6)}" y1="{f(Y(1.05))}" x2="{f(cx - half - 2)}" y2="{f(Y(1.05))}" style="stroke:#FFFFFF;stroke-width:2;opacity:0.8"/>')
    p.append(f'<line x1="{f(cx + half + 2)}" y1="{f(Y(1.05))}" x2="{f(x1 - 6)}" y2="{f(Y(1.05))}" style="stroke:#FFFFFF;stroke-width:2;opacity:0.8"/>')
    # The Starry Night on its glass stele
    p.append(f'<rect x="120" y="150" width="52" height="76" style="fill:#BFE0E0;fill-opacity:0.5;stroke:{TEAL};stroke-width:1.2"/>')
    p.append(f'<rect x="127.6" y="156" width="36.8" height="29.6" style="fill:#1E2A33;stroke:{GILT};stroke-width:1.5"/>')
    p.append('<path d="M 130 178 Q 140 168 152 174 T 162 170" style="fill:none;stroke:#5B82B5;stroke-width:1.6"/>')
    p.append('<circle cx="154" cy="162.8" r="3" style="fill:#E8C46A"/><circle cx="138" cy="162" r="2" style="fill:#E8C46A"/>')
    # visitor 1.75 m
    p.append(f'<circle cx="60" cy="{f(G - 1.75 * s + 6)}" r="6" style="fill:{INK}"/><rect x="54" y="{f(G - 1.75 * s + 13)}" width="12" height="{f(1.75 * s - 13)}" rx="5" style="fill:{INK}"/>')
    t = []
    t.append('<text x="146" y="144" class="lbl-g mid">THE STARRY NIGHT</text>')
    t.append('<text x="146" y="244" class="lbl mid">GLASS STELE</text>')
    t.append(f'<text x="{cx}" y="244" class="lbl-t mid">ROUND GLASS CAR · Ø 4.4 M</text>')
    t.append('<text x="60" y="244" class="lbl mid">1.75 M</text>')
    t.append(f'<text x="{cx + 10}" y="14" class="lbl-t">↑ THE SPHERE</text>')
    t.append(f'<text x="{f(cx - ew / 2 - 8)}" y="{f(Y(SCREEN_H) + 4)}" class="lbl end">GLASS ENCLOSURE</text>')
    return ('<svg viewBox="0 0 512 250" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="Elevation at one scale: '
            'The Starry Night on its glass stele; the round glass car, 4.4 m across, with bronze rings, a luminous canopy and curved doors '
            'that slide round it, inside its glass enclosure; a visitor for scale">' + "".join(p) + "".join(t) + "</svg>")


def atrium_half_section():
    s = 9.0
    G = 214.0
    AX = 36.0
    def X(r): return AX + r * s
    def Y(z): return G - z * s
    p = []
    top = 6.0
    # the Atrium's space
    p.append(f'<rect x="{AX}" y="{f(Y(C))}" width="{f(R * s)}" height="{f(C * s)}" style="fill:{CARD}"/>')
    # the Sphere's inside above its underside (the sky)
    Rs = R * s
    p.append(f'<path d="M {AX} {f(Y(RING_Z))} L {f(X(RING_R))} {f(Y(RING_Z))} A {f(Rs)} {f(Rs)} 0 0 0 {f(X(R))} {f(Y(C))} L {f(X(R))} {top} L {AX} {top} Z" style="fill:{NIGHT}"/>')
    rng = random.Random(3)
    def inside(x, y):
        dx, dz = (x - AX) / s, (G - y) / s - C
        return dx * dx + dz * dz < (R - 0.8) ** 2 and (G - y) / s > RING_Z + 1 and y > top + 3
    p.append(stars(rng, 16, (lambda r: (r.uniform(AX + 4, X(R) - 4), r.uniform(top, Y(RING_Z))), inside), r=1.0))
    # soft daylight through the drum onto the floor
    p.append(f'<polygon points="{f(X(R))},{f(Y(BASE))} {f(X(R))},{f(Y(C) + 20)} {f(X(4))},{G} {f(X(10))},{G}" style="fill:#F4E9C8;opacity:0.35"/>')
    # ribs: up the drum, then under the Sphere to the ring round the opening
    D = 0.5
    haunch = math.acos((R - D) / (R + D))
    cz = C - (R + D) * math.sin(haunch)
    zi = C - math.sqrt((R + D) ** 2 - RING_R ** 2)
    Ri = (R + D) * s
    p.append(f'<path d="M {f(X(R))} {f(Y(BASE))} L {f(X(R))} {f(Y(C))} A {f(Rs)} {f(Rs)} 0 0 1 {f(X(RING_R))} {f(Y(RING_Z))} '
             f'L {f(X(RING_R))} {f(Y(zi))} A {f(Ri)} {f(Ri)} 0 0 0 {f(X(R - D))} {f(Y(cz))} L {f(X(R - D))} {f(Y(BASE))} Z" style="fill:{RIB}"/>')
    # compression ring round the opening
    p.append(f'<rect x="{f(X(RING_R))}" y="{f(Y(RING_Z) - 1)}" width="{f(0.6 * s)}" height="6" style="fill:{RIB}"/>')
    # drum (misty glass) and travertine base
    p.append(f'<rect x="{f(X(R))}" y="{f(Y(C))}" width="4" height="{f((C - BASE) * s)}" style="fill:#D5E4E3;stroke:{TEAL};stroke-width:1"/>')
    p.append(f'<line x1="{f(X(R))}" y1="{top}" x2="{f(X(R))}" y2="{f(Y(C))}" style="stroke:#9FB5B5;stroke-width:3"/>')
    p.append(f'<rect x="{f(X(R))}" y="{f(Y(BASE))}" width="{f(WALL * s)}" height="{f(BASE * s)}" style="fill:{TRAV};stroke:{INK};stroke-width:1.2"/>')
    p.append(f'<rect x="{f(X(R) - 2.5)}" y="{f(Y(3.4))}" width="2.5" height="{f(2.0 * s)}" style="fill:{GILT}"/>')
    # floor
    p.append(f'<line x1="26" y1="{G}" x2="{f(X(R + WALL) + 6)}" y2="{G}" style="stroke:{INK};stroke-width:1.5"/>')
    # the car at the Atrium, its enclosure, the line of the ride and the bronze mast in the Sphere
    p.append(f'<rect x="{AX}" y="{f(Y(CAR_H))}" width="{f(CAR_R * s)}" height="{f(CAR_H * s)}" style="fill:#BFE0E0;stroke:{TEAL};stroke-width:1.3"/>')
    p.append(f'<line x1="{f(X(SCREEN_R))}" y1="{f(Y(SCREEN_H))}" x2="{f(X(SCREEN_R))}" y2="{G}" style="stroke:{TEAL};stroke-width:1"/>')
    p.append(f'<line x1="{AX}" y1="{f(Y(CAR_H))}" x2="{AX}" y2="{f(Y(POLE))}" style="stroke:{TEAL};stroke-width:1.4;stroke-dasharray:4 3"/>')
    p.append(f'<line x1="{AX}" y1="{f(Y(POLE))}" x2="{AX}" y2="{top}" style="stroke:{GILT_L};stroke-width:2.2"/>')
    # bronze ring on the floor, under the opening
    p.append(f'<rect x="{f(X(RING_R) - 1)}" y="{G - 2}" width="5" height="2" style="fill:{GILT}"/>')
    # visitor
    vx = X(9.5)
    p.append(f'<circle cx="{f(vx)}" cy="{f(G - 1.75 * s + 1.8)}" r="1.8" style="fill:{INK}"/><rect x="{f(vx - 1.6)}" y="{f(G - 1.75 * s + 4)}" width="3.2" height="{f(1.75 * s - 4)}" rx="1.2" style="fill:{INK}"/>')
    t = []
    L = 186
    t.append(f'<text x="{L}" y="{f(Y(C) + 4)}" class="lbl">EQUATOR ON THE DRUM · 22 M</text>')
    t.append(f'<text x="{L}" y="{f(Y(15.5))}" class="lbl">MISTY GLASS DRUM</text>')
    t.append(f'<text x="{L}" y="{f(Y(12.2))}" class="lbl">RIBS TURN UNDER THE SPHERE</text>')
    t.append(f'<text x="{L}" y="{f(Y(RING_Z) + 4)}" class="lbl-t">OPENING · Ø 6 M · 8.3 M</text>')
    t.append(f'<text x="{L}" y="{f(Y(BASE) + 4)}" class="lbl">TRAVERTINE BASE · 6 M</text>')
    t.append(f'<text x="{L}" y="{f(Y(2.4))}" class="lbl">STONE WALL · NEW WORK</text>')
    t.append(f'<text x="{AX}" y="232" class="lbl-t">↓ THE SQUARE</text>')
    t.append(f'<text x="{AX + 120}" y="232" class="lbl">CENTRE → WALL, 14 M</text>')
    return ('<svg viewBox="0 0 400 238" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="Half section at true scale from the car to the wall: '
            'a 6 m travertine base, a drum of misty glass to 22 m, and the Sphere sitting on it, its underside the ceiling down to the opening over the car, '
            'Ø 6 m at 8.3 m; twelve ribs rise up the drum and turn under the Sphere to a ring round the opening">' + "".join(p) + "".join(t) + "</svg>")


# ---------------------------------------------------------------- Élan section at true scale, looking north

def elan_section():
    s = 12.5
    XA = 54.0
    def X(x): return 74 + (x - 26.0) * s
    def Y(z): return 44 + (37.0 - z) * s
    p = []
    W = X(70.5)
    # earth and ground
    p.append(f'<rect x="{f(X(26))}" y="{f(Y(0))}" width="{f(W - X(26))}" height="{f(Y(-11.2) - Y(0))}" style="fill:{TRAV}"/>')
    # the Square: rammed-earth walls 1.2 m, floor −9, ceiling −1.5
    sx0, sx1 = XA - 14, XA + 14
    p.append(f'<rect x="{f(X(sx0 - 1.2))}" y="{f(Y(SQ_CEIL))}" width="{f((28 + 2.4) * s)}" height="{f((SQ_CEIL - SQ_FLOOR + 0.4) * s)}" style="fill:#A07F5A;stroke:{INK};stroke-width:1"/>')
    p.append(f'<rect x="{f(X(sx0))}" y="{f(Y(SQ_CEIL))}" width="{f(28 * s)}" height="{f((SQ_CEIL - SQ_FLOOR) * s)}" style="fill:#F1E9DC"/>')
    for i in range(1, 4):  # strata lines in the walls
        zz = SQ_FLOOR + i * 1.9
        for xa, xb in ((sx0 - 1.2, sx0), (sx1, sx1 + 1.2)):
            p.append(f'<line x1="{f(X(xa))}" y1="{f(Y(zz))}" x2="{f(X(xb))}" y2="{f(Y(zz))}" style="stroke:#7E6446;stroke-width:0.8"/>')
    # the glass floor over the strata, on square 5
    gx0, gx1 = XA - 28 / 6, XA + 28 / 6
    bands = [("#8C6B4A", 0.6), ("#B08D64", 0.6), ("#E9E2D2", 0.5), ("#8F8A80", 0.5)]
    zz = SQ_FLOOR - 0.05
    for col, th in bands:
        p.append(f'<rect x="{f(X(gx0))}" y="{f(Y(zz))}" width="{f((gx1 - gx0) * s)}" height="{f(th * s)}" style="fill:{col}"/>')
        zz -= th
    p.append(f'<line x1="{f(X(gx0))}" y1="{f(Y(SQ_FLOOR))}" x2="{f(X(gx1))}" y2="{f(Y(SQ_FLOOR))}" style="stroke:{TEAL};stroke-width:2"/>')
    # slab between the Square and the Atrium, with the shaft through it
    p.append(f'<rect x="{f(X(sx0 - 1.2))}" y="{f(Y(0))}" width="{f((XA - SCREEN_R - sx0 + 1.2) * s)}" height="{f(1.5 * s)}" style="fill:#CFC6B8;stroke:{INK};stroke-width:1"/>')
    p.append(f'<rect x="{f(X(XA + SCREEN_R))}" y="{f(Y(0))}" width="{f((sx1 + 1.2 - XA - SCREEN_R) * s)}" height="{f(1.5 * s)}" style="fill:#CFC6B8;stroke:{INK};stroke-width:1"/>')
    # Hall of Light (a stub, west): floor 0, glass walls 5 m, lattice crown 7.5 m
    hx0, hx1 = 26.0, XA - R - WALL
    p.append(f'<rect x="{f(X(hx0))}" y="{f(Y(7.5))}" width="{f((hx1 - hx0) * s)}" height="{f(7.5 * s)}" style="fill:{MIST}"/>')
    for k in range(0, 5):
        xm = hx1 - 3 * k - 0.2
        if xm > hx0:
            p.append(f'<line x1="{f(X(xm))}" y1="{f(Y(5))}" x2="{f(X(xm))}" y2="{f(Y(0))}" style="stroke:{TEAL};stroke-width:0.8;opacity:0.7"/>')
    xx = hx0
    while xx < hx1 - 0.1:
        p.append(f'<path d="M {f(X(xx))} {f(Y(5))} L {f(X(min(xx + 1.5, hx1)))} {f(Y(7.5))} L {f(X(min(xx + 3, hx1)))} {f(Y(5))}" style="fill:none;stroke:{TEAL};stroke-width:0.6;opacity:0.6"/>')
        xx += 3
    p.append(f'<line x1="{f(X(hx0))}" y1="{f(Y(7.5))}" x2="{f(X(hx1))}" y2="{f(Y(7.5))}" style="stroke:#3A3A3A;stroke-width:2"/>')
    p.append(f'<line x1="{f(X(hx0))}" y1="{f(Y(5))}" x2="{f(X(hx1))}" y2="{f(Y(5))}" style="stroke:{TEAL};stroke-width:1"/>')
    p.append(f'<text x="{f((X(hx0) + X(hx1)) / 2)}" y="{f(Y(2.2))}" class="lbl mid">HALL OF LIGHT</text>')
    # the Atrium: space, stone base, glass drum
    p.append(f'<rect x="{f(X(XA - R))}" y="{f(Y(C))}" width="{f(2 * R * s)}" height="{f(C * s)}" style="fill:{CARD}"/>')
    p.append(f'<polygon points="{f(X(XA - R))},{f(Y(BASE))} {f(X(XA - R))},{f(Y(C - 2))} {f(X(XA - 4))},{f(Y(0))} {f(X(XA - 11))},{f(Y(0))}" style="fill:#F4E9C8;opacity:0.35"/>')
    p.append(f'<polygon points="{f(X(XA + R))},{f(Y(BASE))} {f(X(XA + R))},{f(Y(C - 2))} {f(X(XA + 4))},{f(Y(0))} {f(X(XA + 11))},{f(Y(0))}" style="fill:#F4E9C8;opacity:0.35"/>')
    # the Sphere: the sky inside, the shell with its opening at the south pole
    Rs = R * s
    cxs, cys = X(XA), Y(C)
    clip = f'<clipPath id="elan-above-ring"><rect x="0" y="0" width="{f(W)}" height="{f(Y(RING_Z))}"/></clipPath>'
    p.append(f'<defs>{clip}</defs>')
    p.append(f'<circle cx="{f(cxs)}" cy="{f(cys)}" r="{f(Rs)}" style="fill:{NIGHT}" clip-path="url(#elan-above-ring)"/>')
    rng = random.Random(11)
    p.append(stars(rng, 120, (lambda r: (r.uniform(cxs - Rs, cxs + Rs), r.uniform(cys - Rs, cys + Rs)),
                               lambda x, y: (x - cxs) ** 2 + (y - cys) ** 2 < (Rs - 6) ** 2 and y < Y(RING_Z) - 6 and (x - cxs) ** 2 + (y - cys) ** 2 > 18 ** 2), r=1.1))
    for k in range(12):
        a = math.radians(15 + 30 * k)
        x2, y2 = cxs + (Rs - 3) * math.cos(a), cys + (Rs - 3) * math.sin(a)
        if y2 > Y(RING_Z) - 2:
            continue
        p.append(f'<line x1="{f(cxs + 14 * math.cos(a))}" y1="{f(cys + 14 * math.sin(a))}" x2="{f(x2)}" y2="{f(y2)}" style="stroke:{GLASS};stroke-width:0.8;stroke-dasharray:3 5;opacity:0.55"/>')
    # ribs under the Sphere and up the drum (both sides)
    D = 0.5
    haunch = math.acos((R - D) / (R + D))
    cz = C - (R + D) * math.sin(haunch)
    zi = C - math.sqrt((R + D) ** 2 - RING_R ** 2)
    Ri = (R + D) * s
    for sgn in (1, -1):
        sw1 = 1 if sgn > 0 else 0
        sw2 = 0 if sgn > 0 else 1
        p.append(f'<path d="M {f(X(XA + sgn * R))} {f(Y(BASE))} L {f(X(XA + sgn * R))} {f(Y(C))} A {f(Rs)} {f(Rs)} 0 0 {sw1} {f(X(XA + sgn * RING_R))} {f(Y(RING_Z))} '
                 f'L {f(X(XA + sgn * RING_R))} {f(Y(zi))} A {f(Ri)} {f(Ri)} 0 0 {sw2} {f(X(XA + sgn * (R - D)))} {f(Y(cz))} L {f(X(XA + sgn * (R - D)))} {f(Y(BASE))} Z" style="fill:{RIB}"/>')
    # shell (misty glass outside), open at the south pole
    a0 = math.asin(RING_R / R)
    p.append(f'<path d="M {f(X(XA - RING_R))} {f(Y(RING_Z))} A {f(Rs)} {f(Rs)} 0 1 1 {f(X(XA + RING_R))} {f(Y(RING_Z))}" style="fill:none;stroke:#DCE7E5;stroke-width:5"/>')
    p.append(f'<path d="M {f(X(XA - RING_R))} {f(Y(RING_Z))} A {f(Rs + 2.5)} {f(Rs + 2.5)} 0 1 1 {f(X(XA + RING_R))} {f(Y(RING_Z))}" style="fill:none;stroke:#8FA9A9;stroke-width:1"/>')
    # walls: travertine base and misty glass drum
    for sgn in (-1, 1):
        xin = XA + sgn * R
        xo = XA + sgn * (R + WALL)
        xl = min(X(xin), X(xo))
        if sgn < 0:   # the west door from the Hall of Light, 4.5 m high
            p.append(f'<rect x="{f(xl)}" y="{f(Y(BASE))}" width="{f(WALL * s)}" height="{f((BASE - 4.5) * s)}" style="fill:{TRAV};stroke:{INK};stroke-width:1"/>')
        else:
            p.append(f'<rect x="{f(xl)}" y="{f(Y(BASE))}" width="{f(WALL * s)}" height="{f(BASE * s)}" style="fill:{TRAV};stroke:{INK};stroke-width:1"/>')
        xg = X(xin) if sgn > 0 else X(xin) - 4
        p.append(f'<rect x="{f(xg)}" y="{f(Y(C))}" width="4" height="{f((C - BASE) * s)}" style="fill:#D5E4E3;stroke:{TEAL};stroke-width:1"/>')
    # floor line of the Atrium
    p.append(f'<line x1="{f(X(26))}" y1="{f(Y(0))}" x2="{f(W)}" y2="{f(Y(0))}" style="stroke:{INK};stroke-width:1.5"/>')
    # the ride: shaft line, mast, enclosures, car stops
    p.append(f'<line x1="{f(cxs)}" y1="{f(Y(SQ_FLOOR))}" x2="{f(cxs)}" y2="{f(Y(POLE))}" style="stroke:{TEAL};stroke-width:1.4;stroke-dasharray:4 3"/>')
    p.append(f'<line x1="{f(cxs)}" y1="{f(Y(POLE))}" x2="{f(cxs)}" y2="{f(Y(TOP_FLOOR))}" style="stroke:{GILT_L};stroke-width:2.6"/>')
    for z0 in (0.0, SQ_FLOOR):
        for sgn in (-1, 1):
            p.append(f'<line x1="{f(X(XA + sgn * SCREEN_R))}" y1="{f(Y(z0 + SCREEN_H))}" x2="{f(X(XA + sgn * SCREEN_R))}" y2="{f(Y(z0))}" style="stroke:{TEAL};stroke-width:1"/>')
    cw = 2 * CAR_R * s
    for z0 in (SQ_FLOOR, 0.0, HOME):
        p.append(f'<rect x="{f(cxs - cw / 2)}" y="{f(Y(z0 + CAR_H))}" width="{f(cw)}" height="{f(CAR_H * s)}" style="fill:none;stroke:{GLASS if z0 == HOME else TEAL};stroke-width:1;stroke-dasharray:3 2"/>')
    # the car at the centre, with a visitor whose eyes are at the Sphere's centre
    p.append(f'<circle cx="{f(cxs)}" cy="{f(cys)}" r="22" style="fill:{GLASS};opacity:0.16"/>')
    p.append(f'<rect x="{f(cxs - cw / 2)}" y="{f(Y(TOP_FLOOR + CAR_H))}" width="{f(cw)}" height="{f(CAR_H * s)}" style="fill:#BFE0E0;fill-opacity:0.35;stroke:{GLASS};stroke-width:1.4"/>')
    p.append(f'<line x1="{f(cxs - cw / 2)}" y1="{f(Y(TOP_FLOOR))}" x2="{f(cxs + cw / 2)}" y2="{f(Y(TOP_FLOOR))}" style="stroke:{GILT_L};stroke-width:2"/>')
    vx = cxs + 13
    p.append(f'<circle cx="{f(vx)}" cy="{f(Y(TOP_FLOOR + 1.75) + 2.2)}" r="2.2" style="fill:#F1EBDF"/><rect x="{f(vx - 2)}" y="{f(Y(TOP_FLOOR + 1.75) + 5)}" width="4" height="{f(1.75 * s - 5)}" rx="1.6" style="fill:#F1EBDF"/>')
    p.append(f'<circle cx="{f(cxs)}" cy="{f(cys)}" r="2" style="fill:#F1EBDF"/>')
    # levels, left
    ticks = []
    for z, lab in ((36, "36 M"), (C, "22 M"), (RING_Z, "8.3 M"), (0, "±0"), (SQ_FLOOR, "−9 M")):
        ticks.append(f'<line x1="6" y1="{f(Y(z))}" x2="16" y2="{f(Y(z))}" style="stroke:{INK};stroke-width:1"/>')
        ticks.append(f'<text x="22" y="{f(Y(z) + 4)}" class="lbl">{lab}</text>')
        ticks.append(f'<line x1="72" y1="{f(Y(z))}" x2="{f(X(XA - R) - 6)}" y2="{f(Y(z))}" style="stroke:#B9AE9B;stroke-width:0.7;stroke-dasharray:1 3"/>' if z > 8 else "")
    p.append(f'<line x1="11" y1="{f(Y(36))}" x2="11" y2="{f(Y(SQ_FLOOR))}" style="stroke:{INK};stroke-width:1"/>')
    # labels with leaders, right
    L = f(X(70.5) + 18)
    lx = X(70.5) + 12
    lab = []
    def lead(x, y, ty):
        lab.append(f'<line x1="{f(x)}" y1="{f(y)}" x2="{f(lx)}" y2="{f(ty - 4)}" style="stroke:#8A8172;stroke-width:0.8"/>')
        lab.append(f'<circle cx="{f(x)}" cy="{f(y)}" r="2.2" style="fill:#8A8172"/>')
    def txt(y, lines, cls="lbl"):
        for i, t in enumerate(lines):
            lab.append(f'<text x="{L}" y="{f(y + 15 * i)}" class="{cls}">{t}</text>')
    a = math.radians(-40)
    lead(cxs + (Rs + 3) * math.cos(a), cys + (Rs + 3) * math.sin(a), 96)
    txt(96, ["THE SPHERE · Ø 28 M", "CENTRE 22 M UP", "FROM OUTSIDE, A MISTY", "PEARL ON THE DRUM, 36 M"])
    lead(cxs + cw / 2, Y(TOP_FLOOR + 1.3), 214)
    txt(214, ["THE CAR STOPS WITH", "YOUR EYES AT THE", "CENTRE, 22 M UP"], "lbl-t")
    lead(cxs + 1, Y(15.0), 290)
    txt(290, ["BRONZE MAST, FROM", "THE SOUTH POLE"], "lbl-g")
    lead(X(XA + RING_R) + 2, Y(RING_Z) + 1, 356)
    txt(356, ["THE OPENING · Ø 6 M", "AT 8.3 M: THE CAR", "WAITS JUST INSIDE"], "lbl-t")
    lead(X(XA + R) + 4, Y(16), 170)
    txt(170, ["GLASS DRUM · 6–22 M"])
    lead(X(XA + R + WALL), Y(3.0), 442)
    txt(442, ["THE ATRIUM · 0", "TRAVERTINE BASE 6 M"])
    lead(X(XA + 11), Y(-5.5), 520)
    txt(520, ["THE SQUARE · −9 M", "RAMMED EARTH"])
    hdr = [f'<text x="14" y="16" class="lbl">SECTION · TRUE SCALE · LOOKING NORTH</text>',
           f'<text x="14" y="33" class="lbl-t">DASHED BLUE: EVERY LINE OF SIGHT MEETS THE SHELL SQUARE-ON</text>']
    sb = (f'<rect x="{f(X(26.5))}" y="{f(Y(-11.2) + 16)}" width="{f(5 * s)}" height="5" style="fill:{INK}"/>'
          f'<rect x="{f(X(31.5))}" y="{f(Y(-11.2) + 16)}" width="{f(5 * s)}" height="5" style="fill:none;stroke:{INK};stroke-width:1"/>'
          f'<text x="{f(X(36.5) + 8)}" y="{f(Y(-11.2) + 22)}" class="lbl">10 M</text>')
    H = Y(-11.2) + 30
    return (f'<svg viewBox="0 0 {f(X(70.5) + 210)} {f(H)}" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="Section at true scale, looking north: '
            f'the Square at minus 9 m, the Atrium, its drum and the 28 m Sphere sitting on it, the bronze mast from the south pole to the car at the centre, 22 m up">'
            + "".join(p) + "".join(ticks) + "".join(lab) + "".join(hdr) + sb + "</svg>")


# ---------------------------------------------------------------- three spheres at one scale

def spheres_compare():
    s = 1.42
    G = 246.0
    p = [f'<line x1="0" y1="{G}" x2="760" y2="{G}" style="stroke:{INK};stroke-width:1.5"/>']
    # Boullée, 1784: Ø 150 m on a two-step podium with cypresses
    bx = 150.0
    p.append(f'<rect x="{f(bx - 100 * s)}" y="{f(G - 8 * s)}" width="{f(200 * s)}" height="{f(8 * s)}" style="fill:{TRAV};stroke:{INK};stroke-width:1"/>')
    p.append(f'<rect x="{f(bx - 88 * s)}" y="{f(G - 16 * s)}" width="{f(176 * s)}" height="{f(8 * s)}" style="fill:{TRAV};stroke:{INK};stroke-width:1"/>')
    for dx in (-96, -86, 86, 96):
        x = bx + dx * s
        p.append(f'<polygon points="{f(x - 3)},{f(G - 16 * s)} {f(x + 3)},{f(G - 16 * s)} {f(x)},{f(G - 16 * s - 20)}" style="fill:#5E6B4F"/>')
    bc = G - (16 + 75) * s
    p.append(f'<circle cx="{bx}" cy="{f(bc)}" r="{f(75 * s)}" style="fill:{NIGHT};stroke:{INK};stroke-width:2"/>')
    rng = random.Random(5)
    rr = 75 * s
    p.append(stars(rng, 40, (lambda r: (r.uniform(bx - rr, bx + rr), r.uniform(bc - rr, bc + rr)),
                              lambda x, y: (x - bx) ** 2 + (y - bc) ** 2 < (rr - 5) ** 2), r=0.9))
    # Las Vegas, 2023: Ø 157 m, 112 m tall
    lx = 442.0
    lr = 78.5 * s
    lc = G - (112 - 78.5) * s
    hw = math.sqrt(78.5 ** 2 - 33.5 ** 2) * s
    p.append(f'<path d="M {f(lx - hw)} {G} A {f(lr)} {f(lr)} 0 1 1 {f(lx + hw)} {G} Z" style="fill:#DCD3C3;stroke:{INK};stroke-width:2"/>')
    for k in range(1, 7):
        y = G - k * 16 * s
        if y < lc - lr + 4:
            break
        dy = y - lc
        w = math.sqrt(max(lr * lr - dy * dy, 0))
        p.append(f'<line x1="{f(lx - w)}" y1="{f(y)}" x2="{f(lx + w)}" y2="{f(y)}" style="stroke:#B9AE9B;stroke-width:1"/>')
    # Élan, 2026: Ø 28 m on its 22 m drum
    ex = 668.0
    er = R * s
    p.append(f'<circle cx="{ex}" cy="{f(G - C * s)}" r="{f(er)}" style="fill:#E6ECEA;stroke:#8FA9A9;stroke-width:1.2"/>')
    p.append(f'<rect x="{f(ex - er)}" y="{f(G - C * s)}" width="{f(2 * er)}" height="{f((C - BASE) * s)}" style="fill:{MIST};stroke:{TEAL};stroke-width:1.2"/>')
    p.append(f'<rect x="{f(ex - er)}" y="{f(G - BASE * s)}" width="{f(2 * er)}" height="{f(BASE * s)}" style="fill:{TRAV};stroke:{INK};stroke-width:1"/>')
    t = [f'<text x="{bx}" y="{G + 24}" class="lbl mid" style="font-size:17px">BOULLÉE, 1784 · Ø 150 M</text>',
         f'<text x="{bx}" y="{G + 48}" class="lbl mid" style="font-size:17px">NEVER BUILT</text>',
         f'<text x="{lx}" y="{G + 24}" class="lbl mid" style="font-size:17px">LAS VEGAS, 2023</text>',
         f'<text x="{lx}" y="{G + 48}" class="lbl mid" style="font-size:17px">157 M WIDE · 112 M TALL</text>',
         f'<text x="{ex}" y="{G + 24}" class="lbl-t mid" style="font-size:17px">ÉLAN · Ø 28 M</text>',
         f'<text x="{ex}" y="{G + 48}" class="lbl-t mid" style="font-size:17px">36 M HIGH</text>']
    return (f'<svg viewBox="0 0 760 {G + 58}" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="Three spheres at one scale: Boullée\'s of 1784, 150 m across; '
            f'the Las Vegas Sphere of 2023, 157 m wide and 112 m tall; and Élan\'s, 28 m across, sitting on its drum 36 m high">' + "".join(p) + "".join(t) + "</svg>")


# ---------------------------------------------------------------- the ride up: height against time

def ride_profile(total=TOP_FLOOR):
    ta = SPEED / ACC
    da = 0.5 * ACC * ta * ta
    cruise = (total - 2 * da) / SPEED
    T = 2 * ta + cruise
    def z(t):
        if t <= ta: return 0.5 * ACC * t * t
        if t <= ta + cruise: return da + SPEED * (t - ta)
        u = T - t
        return total - 0.5 * ACC * u * u
    def t_at(h):
        if h <= da: return math.sqrt(2 * h / ACC)
        return ta + (h - da) / SPEED
    return T, z, t_at


def ride_chart():
    T, z, t_at = ride_profile()
    t_ring = t_at(RING_Z)
    X0, sx = 84.0, 44.0
    G, sz = 196.0, 7.6
    def X(t): return X0 + t * sx
    def Y(h): return G - h * sz
    p = []
    p.append(f'<rect x="{f(X(t_ring))}" y="{f(Y(TOP_FLOOR + 2.4))}" width="{f(X(T + 3.4) - X(t_ring))}" height="{f(Y(RING_Z) - Y(TOP_FLOOR + 2.4))}" style="fill:{NIGHT};opacity:0.08"/>')
    p.append(f'<text x="{f(X(T + 3.4) - 8)}" y="{f(Y(RING_Z) - 8)}" class="lbl end">INSIDE THE SPHERE</text>')
    p.append(f'<line x1="{X0}" y1="{G}" x2="{f(X(T + 3.4))}" y2="{G}" style="stroke:{INK};stroke-width:1"/>')
    p.append(f'<line x1="{X0}" y1="{G}" x2="{X0}" y2="{f(Y(TOP_FLOOR + 2.4))}" style="stroke:{INK};stroke-width:1"/>')
    for h, lab in ((0, "0"), (RING_Z, "8.3 M"), (TOP_FLOOR, "20.4 M")):
        p.append(f'<line x1="{X0 - 5}" y1="{f(Y(h))}" x2="{X0}" y2="{f(Y(h))}" style="stroke:{INK};stroke-width:1"/>')
        p.append(f'<text x="{X0 - 10}" y="{f(Y(h) + 4)}" class="lbl end">{lab}</text>')
        if h:
            p.append(f'<line x1="{X0}" y1="{f(Y(h))}" x2="{f(X(T + 3.4))}" y2="{f(Y(h))}" style="stroke:#CFC6B8;stroke-width:0.8;stroke-dasharray:2 4"/>')
    pts = " ".join(f"{f(X(t))},{f(Y(z(t)))}" for t in [i * T / 120 for i in range(121)])
    p.append(f'<polyline points="{pts} {f(X(T + 3.4))},{f(Y(TOP_FLOOR))}" style="fill:none;stroke:{TEAL};stroke-width:2.4"/>')
    for t in range(0, 17, 2):
        p.append(f'<line x1="{f(X(t))}" y1="{G}" x2="{f(X(t))}" y2="{G + 4}" style="stroke:{INK};stroke-width:1"/>')
    for t, lab in ((0, "0:00"), (t_ring, f"0:0{round(t_ring)}"), (T, f"0:{round(T)}")):
        p.append(f'<text x="{f(X(t))}" y="{G + 20}" class="lbl mid">{lab}</text>')
    tm = (t_ring + T) / 2
    for t in (0, t_ring, tm, T):
        p.append(f'<circle cx="{f(X(t))}" cy="{f(Y(z(t)))}" r="5" style="fill:#FBF8F2;stroke:{TEAL};stroke-width:2"/>')
    tx = []
    tx.append(f'<text x="{f(X(0) + 14)}" y="{f(Y(15))}" class="lbl-t">THE ATRIUM · 0 M</text>')
    tx.append(f'<text x="{f(X(0) + 14)}" y="{f(Y(15) + 16)}" class="cap">Doors close beside The Starry Night</text>')
    tx.append(f'<text x="{f(X(t_ring) + 16)}" y="{f(Y(RING_Z) + 22)}" class="lbl-t">THROUGH THE OPENING · 8.3 M</text>')
    tx.append(f'<text x="{f(X(t_ring) + 16)}" y="{f(Y(RING_Z) + 38)}" class="cap">The building is gone; the sky is all round, below your feet too</text>')
    tx.append(f'<text x="{f(X(tm) - 14)}" y="{f(Y(z(tm)) - 28)}" class="lbl-t end">RISING THROUGH THE STARS</text>')
    tx.append(f'<text x="{f(X(tm) - 14)}" y="{f(Y(z(tm)) - 12)}" class="cap end">A steady 1.5 m/s, no speeding up</text>')
    tx.append(f'<text x="{f(X(T) + 18)}" y="{f(Y(TOP_FLOOR) - 26)}" class="lbl-t">THE CENTRE · EYES AT 22 M</text>')
    tx.append(f'<text x="{f(X(T) + 18)}" y="{f(Y(TOP_FLOOR) - 10)}" class="cap">The car stops; its glass dims to a rail and a floor</text>')
    tx.append(f'<text x="{X0}" y="14" class="lbl">THE RIDE UP · THE CAR\'S FLOOR, HEIGHT AGAINST TIME</text>')
    W = X(T + 3.4) + 360
    return (f'<svg viewBox="0 0 {f(W)} 226" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="The ride from the Atrium to the centre of the Sphere: '
            f'about {round(T)} seconds at a steady 1.5 metres a second, through the opening at 8.3 m after about {round(t_ring)} seconds">' + "".join(p) + "".join(tx) + "</svg>"), T, t_ring


# ---------------------------------------------------------------- Master plan inset: level 2

def masterplan_level2():
    p = ['<svg viewBox="0 0 230 190" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="Level 2: the Sphere, 28 m across, exactly over the 28 m Atrium, drawn at the same scale">']
    cx, cy, r = 115, 102, 70
    p.append(f'<circle cx="{cx}" cy="{cy}" r="{r}" style="fill:{NIGHT}"/>')
    rng = random.Random(2)
    p.append(stars(rng, 9, (lambda q: (q.uniform(cx - r, cx + r), q.uniform(cy - r, cy + r)),
                            lambda x, y: (x - cx) ** 2 + (y - cy) ** 2 < (r - 8) ** 2 and (x - cx) ** 2 + (y - cy) ** 2 > 22 ** 2), r=1.4))
    p.append(f'<circle cx="{cx}" cy="{cy}" r="{r}" style="fill:none;stroke:{GLASS};stroke-width:1.2;stroke-dasharray:3 3"/>')
    ro = r * RING_R / R
    p.append(f'<circle cx="{cx}" cy="{cy}" r="{f(ro)}" style="fill:none;stroke:{GLASS};stroke-width:1.2"/>')
    p.append(f'<circle cx="{cx}" cy="{cy}" r="3" style="fill:{GLASS}"/>')
    p.append('<text x="115" y="16" class="lbl mid">SAME SCALE</text>')
    p.append('</svg>')
    return "".join(p)


# ---------------------------------------------------------------- On a PC: the keys

def pc_keys():
    u = 46.0; gap = 6.0
    p = []
    def key(x, y, w, label, on=False, sub=None, small=False):
        fill = "#FBF8F2" if not on else "#1E1C19"
        col = "#5E574D" if not on else "#F1EBDF"
        p.append(f'<rect x="{f(x)}" y="{f(y)}" width="{f(w)}" height="{u}" rx="7" style="fill:{fill};stroke:{"#CFC6B8" if not on else "#1E1C19"};stroke-width:1.2"/>')
        fs = 12 if small else 15
        p.append(f'<text x="{f(x + w / 2)}" y="{f(y + u / 2 + 5)}" style="font-family:var(--mono);font-size:{fs}px;fill:{col};text-anchor:middle;letter-spacing:0.04em">{label}</text>')
    x0, y0 = 20.0, 60.0
    rows = [
        (0, ["Esc", "", "1", "2", "3", "4", "5", "6"]),
        (1, ["Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"]),
        (2, ["Caps", "A", "S", "D", "F", "G", "H", "J", "K", "L"]),
    ]
    on = {"Esc", "1", "2", "3", "4", "W", "E", "T", "P", "A", "S", "D"}
    # row 0: Esc, then digits
    key(x0, y0, u, "Esc", True, small=True)
    for i, k in enumerate(["1", "2", "3", "4", "5", "6", "7"]):
        key(x0 + (i + 1) * (u + gap) + 14, y0, u, k, k in on)
    y = y0 + u + gap
    key(x0, y, u * 1.4, "Tab", small=True)
    for i, k in enumerate(["Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"]):
        key(x0 + u * 1.4 + gap + i * (u + gap), y, u, k, k in on)
    y += u + gap
    key(x0, y, u * 1.7, "Caps", small=True)
    for i, k in enumerate(["A", "S", "D", "F", "G", "H", "J", "K", "L"]):
        key(x0 + u * 1.7 + gap + i * (u + gap), y, u, k, k in on)
    # mouse
    mx, my = 640.0, 58.0
    p.append(f'<rect x="{mx}" y="{my}" width="78" height="120" rx="39" style="fill:#FBF8F2;stroke:#CFC6B8;stroke-width:1.2"/>')
    p.append(f'<line x1="{mx + 39}" y1="{my}" x2="{mx + 39}" y2="{my + 46}" style="stroke:#CFC6B8;stroke-width:1.2"/>')
    p.append(f'<line x1="{mx}" y1="{my + 46}" x2="{mx + 78}" y2="{my + 46}" style="stroke:#CFC6B8;stroke-width:1.2"/>')
    p.append(f'<path d="M {mx + 39} {my} L {mx + 39} {my + 46} L {mx} {my + 46} L {mx} {my + 39} A 39 39 0 0 1 {mx + 39} {my} Z" style="fill:#1E1C19"/>')
    # gamepad
    gx, gy = 780.0, 70.0
    p.append(f'<path d="M {gx + 30} {gy} L {gx + 130} {gy} C {gx + 160} {gy} {gx + 172} {gy + 30} {gx + 176} {gy + 70} C {gx + 180} {gy + 100} {gx + 158} {gy + 108} {gx + 144} {gy + 90} L {gx + 128} {gy + 70} L {gx + 32} {gy + 70} L {gx + 16} {gy + 90} C {gx + 2} {gy + 108} {gx - 20} {gy + 100} {gx - 16} {gy + 70} C {gx - 12} {gy + 30} {gx} {gy} {gx + 30} {gy} Z" style="fill:#FBF8F2;stroke:#CFC6B8;stroke-width:1.2"/>')
    p.append(f'<circle cx="{gx + 36}" cy="{gy + 34}" r="14" style="fill:#1E1C19"/>')
    p.append(f'<circle cx="{gx + 104}" cy="{gy + 50}" r="11" style="fill:none;stroke:#CFC6B8;stroke-width:1.2"/>')
    for dx, dy in ((0, -12), (12, 0), (0, 12), (-12, 0)):
        p.append(f'<circle cx="{gx + 134 + dx}" cy="{gy + 32 + dy}" r="4.5" style="fill:none;stroke:#CFC6B8;stroke-width:1.1"/>')
    # captions
    t = []
    t.append(f'<text x="{x0}" y="30" class="lbl">THE KEYS YOU NEED, IN BLACK</text>')
    t.append(f'<text x="{mx + 39}" y="{my + 146}" class="lbl mid">LOOK</text>')
    t.append(f'<text x="{mx + 39}" y="{my + 162}" class="lbl mid">CLICK TO USE</text>')
    t.append(f'<text x="{gx + 80}" y="{my + 146}" class="lbl mid">OR A GAMEPAD</text>')
    t.append(f'<text x="{gx + 80}" y="{my + 162}" class="lbl mid">LEFT STICK WALKS</text>')
    return ('<svg viewBox="0 0 980 240" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="The keys: W A S D to walk, E to use, 1 to 4 for buttons, T for the time of day, P for photo mode, Esc to quit; the mouse to look and click; or a gamepad">'
            + "".join(p) + "".join(t) + "</svg>")


if __name__ == "__main__":
    import sys
    name = sys.argv[1] if len(sys.argv) > 1 else "elan_section"
    out = globals()[name]()
    if isinstance(out, tuple):
        svg, T, t_ring = out
        sys.stderr.write(f"ride: {T:.1f} s to the centre, {t_ring:.1f} s to the opening\n")
        out = svg
    sys.stdout.buffer.write(out.encode("utf-8"))
