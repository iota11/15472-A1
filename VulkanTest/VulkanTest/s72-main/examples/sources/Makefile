BLENDER=C:\Program Files\Blender Foundation\Blender 4.0\blender.exe
EXPORT_PY=../../exporters/blender/export-s72.py

all : \
	../sg-Articulation.s72 \
	../sg-Support.s72 \
	../sg-Containment.s72 \
	../sg-Grouping.s72 \
	../sphereflake.s72 \
	../sg-cube.s72 \


../sg-Articulation.s72 : scene-graphs.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection Articulation --animate 1 91 '$@'

../sg-Support.s72 : scene-graphs.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection Support '$@'

../sg-Containment.s72 : scene-graphs.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection Containment '$@'

../sg-Grouping.s72 : scene-graphs.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection Grouping --animate 1 51 '$@'

../sphereflake.s72 : sphereflake.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection Base '$@'

../sg-cube.s72 :  scene-graphs.blend $(EXPORT_PY)
	$(BLENDER) --background --python '$(EXPORT_PY)' -- '$<' --collection Cube '$@'