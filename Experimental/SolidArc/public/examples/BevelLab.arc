# SolidArc native document v1
# Bevel lab: generic rolling-ball fillet / chamfer proof scene (Kernel/RollingBallBlend).
#   RoundedBlock  all 12 edges fillet 0.2   -> 8 sphere corners
#   ChamferBlock  all 12 edges chamfer 0.15 -> 8 triple mitres
#   BossPlate     plate top edges + concave boss root + boss rim fillet 0.15
#   Puck          both cylinder rims fillet 0.25 (closed seam edges)
#   HoleSlab      top edges + hole rim fillet 0.12
#   Frustum       both cone rims chamfer 0.12
workplane xy
box (0,0,0) (2,1.5,1) --name=RoundedBlock
box (3,0,0) (5,1.5,1) --name=ChamferBlock
box (6,0,0) (9,2,0.6) --name=Plate
cylinder (7.5,1,0.3) 0.5 1.0 --name=Boss
boolean union Plate -- Boss --name=BossPlate
cylinder (11,1,0) 0.7 1.2 --name=Puck
box (13,0,0) (16,2,0.6) --name=Slab
cylinder (14.5,1,-0.5) 0.45 2 --name=Bore
boolean subtract Slab -- Bore --name=HoleSlab
cone (18,1,0) 0.9 0.45 1.1 --name=Frustum
beveladd 0.2 --mode=fillet --targets=1:e0,1,2,3,4,5,6,7,8,9,10,11
beveladd 0.15 --mode=chamfer --targets=2:e0,1,2,3,4,5,6,7,8,9,10,11
beveladd 0.15 --mode=fillet --targets=5:e4,5,6,7,8,14
beveladd 0.25 --mode=fillet --targets=6:e0,2
beveladd 0.12 --mode=fillet --targets=9:e5,6,7,8,9
beveladd 0.12 --mode=chamfer --targets=10:e0,2
show cages off
gizmo off
dim off
view iso
view fit
selectmode whole
