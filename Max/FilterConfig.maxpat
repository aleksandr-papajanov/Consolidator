{
  "patcher": {
    "fileversion": 1,
    "appversion": { "major": 9, "minor": 0, "revision": 9, "architecture": "x64", "modernui": 1 },
    "classnamespace": "box",
    "rect": [120.0, 120.0, 560.0, 260.0],
    "openinpresentation": 1,
    "boxes": [
      { "box": { "id": "obj-1", "maxclass": "inlet", "numinlets": 0, "numoutlets": 1, "outlettype": ["int"], "patching_rect": [40.0, 35.0, 30.0, 30.0], "presentation": 1, "presentation_rect": [20.0, 20.0, 30.0, 30.0] } },
      { "box": { "id": "obj-2", "maxclass": "newobj", "numinlets": 1, "numoutlets": 2, "outlettype": ["bang", "int"], "patching_rect": [40.0, 85.0, 55.0, 22.0], "text": "t b i" } },
      { "box": { "id": "obj-3", "maxclass": "newobj", "numinlets": 1, "numoutlets": 1, "outlettype": [""], "patching_rect": [130.0, 85.0, 190.0, 22.0], "text": "sprintf replace selected %ld" } },
      { "box": { "id": "obj-4", "maxclass": "newobj", "numinlets": 2, "numoutlets": 5, "outlettype": ["dictionary", "", "", "", ""], "patching_rect": [40.0, 145.0, 160.0, 22.0], "text": "dict filterconfig" } },
      { "box": { "id": "obj-5", "maxclass": "outlet", "numinlets": 1, "numoutlets": 0, "outlettype": ["dictionary"], "patching_rect": [40.0, 200.0, 30.0, 30.0], "presentation": 1, "presentation_rect": [20.0, 80.0, 30.0, 30.0] } },
      { "box": { "id": "obj-6", "maxclass": "newobj", "numinlets": 1, "numoutlets": 1, "outlettype": [""], "patching_rect": [260.0, 35.0, 215.0, 22.0], "text": "loadmess read Config/FilterConfig.json" } }
    ],
    "lines": [
      { "patchline": { "destination": ["obj-2", 0], "source": ["obj-1", 0] } },
      { "patchline": { "destination": ["obj-3", 0], "source": ["obj-2", 1] } },
      { "patchline": { "destination": ["obj-4", 0], "source": ["obj-3", 0] } },
      { "patchline": { "destination": ["obj-4", 0], "source": ["obj-2", 0] } },
      { "patchline": { "destination": ["obj-4", 0], "source": ["obj-6", 0] } },
      { "patchline": { "destination": ["obj-5", 0], "source": ["obj-4", 0] } }
    ],
    "dependency_cache": [],
    "autosave": 0
  }
}
