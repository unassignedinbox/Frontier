# SolidArc native document v1
# Minimized reproduction of the user's two earlier fillets and a selected mixed arc/line rim.
# Same exact rectangular extrusion, face displacement and radii as UserCorner.arc.
# The second fillet (vertical edge 3) propagates along its tangent chain: edge 3 -> the first fillet's end arc -> the
#    top-left edge (general rolling-ball route). Its far end leaves arc 10 on the back face, tangent to vertical edge 9
#    and top-back edge 11: the selected rim is arc 10 + line 11.
workplane xy
draw rect (1.108442137,4.463490499) (-4.055601217,1.873156718)
extrude 1 1 --select-result
select 2
transform selected 1 0 0 0 0 1 0 0 0 0 1 0 0 0 2.8458331226924658 1
selectmode face
select none
select faces 2 4 --add
transform selected 1 0 0 0 0 1 0 0 0 0 1 0 0 0 0.86521374798272976 1 --components
transform selected 1 0 0 0 0 1 0 0 0 0 1 0 0 0 2.2543752955710419 1 --components
select none
beveladd 0.25820216770000004 --mode=fillet --targets=2:e2
beveladd 0.19306934379268867 --mode=fillet --targets=2:e3
selectmode edge
select none
select edges 2 10 11 --add
gizmo off
dim hide all
view iso
view fit
