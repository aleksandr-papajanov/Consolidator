{
  "patcher": {
    "fileversion": 1,
    "appversion": { "major": 9, "minor": 0, "revision": 9, "architecture": "x64", "modernui": 1 },
    "classnamespace": "box",
    "rect": [ 236.0, 105.0, 1180.0, 720.0 ],
    "openinpresentation": 1,
    "default_fontsize": 12.0,
    "default_fontname": "Arial",
    "gridsize": [ 15.0, 15.0 ],
    "boxes": [
      { "box": { "id": "plugin", "maxclass": "newobj", "text": "plugin~ 1 2 3 4", "numinlets": 4, "numoutlets": 4, "outlettype": [ "signal", "signal", "signal", "signal" ], "patching_rect": [ 35.0, 35.0, 105.0, 22.0 ] } },
      { "box": { "id": "external", "maxclass": "newobj", "text": "ConsolidatorExternal", "numinlets": 5, "numoutlets": 5, "outlettype": [ "", "signal", "signal", "signal", "signal" ], "patching_rect": [ 250.0, 35.0, 160.0, 22.0 ] } },
      { "box": { "id": "bridge", "maxclass": "bpatcher", "name": "Project:/patchers/ConsolidatorBridge.maxpat", "numinlets": 1, "numoutlets": 2, "outlettype": [ "", "" ], "patching_rect": [ 35.0, 100.0, 1120.0, 520.0 ], "presentation": 1, "presentation_rect": [ 0.0, 0.0, 1120.0, 520.0 ] } },
      { "box": { "id": "plugout", "maxclass": "newobj", "text": "plugout~", "numinlets": 2, "numoutlets": 2, "outlettype": [ "signal", "signal" ], "patching_rect": [ 250.0, 680.0, 60.0, 22.0 ] } }
    ],
    "lines": [
      { "patchline": { "source": [ "plugin", 0 ], "destination": [ "external", 1 ] } },
      { "patchline": { "source": [ "plugin", 1 ], "destination": [ "external", 2 ] } },
      { "patchline": { "source": [ "plugin", 2 ], "destination": [ "external", 3 ] } },
      { "patchline": { "source": [ "plugin", 3 ], "destination": [ "external", 4 ] } },
      { "patchline": { "source": [ "external", 0 ], "destination": [ "bridge", 0 ] } },
      { "patchline": { "source": [ "bridge", 0 ], "destination": [ "external", 0 ] } },
      { "patchline": { "source": [ "external", 1 ], "destination": [ "plugout", 0 ] } },
      { "patchline": { "source": [ "external", 2 ], "destination": [ "plugout", 1 ] } }
    ],
    "parameters": {},
    "dependency_cache": [],
    "latency": 0,
    "minimum_live_version": "",
    "minimum_max_version": "",
    "platform_compatibility": 0,
    "project": {
      "version": 1,
      "creationdate": 3590052493,
      "modificationdate": 3590052493,
      "viewrect": [ 0.0, 0.0, 300.0, 500.0 ],
      "autoorganize": 1,
      "hideprojectwindow": 1,
      "showdependencies": 1,
      "autolocalize": 0,
      "contents": {
        "patchers": {},
        "code": {},
        "externals": {}
      },
      "layout": {},
      "searchpath": {},
      "detailsvisible": 0,
      "amxdtype": 1633771873,
      "readonly": 0,
      "devpathtype": 0,
      "devpath": ".",
      "sortmode": 0,
      "viewmode": 0,
      "includepackages": 0
    },
    "autosave": 0
  }
}
