BLENDER=C:\Program Files\Blender Foundation\Blender 4.0\blender.exe
EXPORT_PY=../../exporters/blender/export-s72.py

all : \
	../sg-Articulation.s72 \
	../sg-Support.s72 \
	../sg-Containment.s72 \
	../sg-Grouping.s72 \
	../balls.s72 \
	../sg-cube.s72 \


../sg-Articulation.s72 : scene-graphs.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection Articulation --animate 1 91 '$@'

../sg-Support.s72 : scene-graphs.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection Support '$@'

../sg-Containment.s72 : scene-graphs.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection Containment '$@'

../sg-Grouping.s72 : scene-graphs.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection Grouping --animate 1 51 '$@'

../balls.s72 : balls.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection balls  --animate 1 60 '$@'

../sg-cube.s72 :  scene-graphs.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection Cube '$@'