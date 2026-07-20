{
  "patcher": {
    "fileversion": 1,
    "appversion": { "major": 9, "minor": 0, "revision": 9, "architecture": "x64", "modernui": 1 },
    "classnamespace": "box",
    "rect": [100.0, 100.0, 360.0, 120.0],
    "boxes": [
      { "box": { "id": "busInput", "maxclass": "newobj", "text": "r ---message.bus.in", "numinlets": 0, "numoutlets": 1, "outlettype": [""], "patching_rect": [30.0, 35.0, 145.0, 22.0] } },
      { "box": { "id": "host", "maxclass": "newobj", "text": "consolidator.devicehost", "numinlets": 2, "numoutlets": 4, "outlettype": ["", "", "", "dictionary"], "patching_rect": [190.0, 35.0, 170.0, 22.0], "saved_object_attributes": { "filename": "consolidator.devicehost.mxe64", "parameter_enable": 0 } } },
      { "box": { "id": "busOutput", "maxclass": "newobj", "text": "s ---message.bus.out", "numinlets": 1, "numoutlets": 0, "patching_rect": [390.0, 35.0, 145.0, 22.0] } },
      { "box": { "id": "persistenceInput", "maxclass": "newobj", "text": "r ---device.persistence.in", "numinlets": 0, "numoutlets": 1, "outlettype": [""], "patching_rect": [30.0, 75.0, 170.0, 22.0] } },
      { "box": { "id": "persistenceOutput", "maxclass": "newobj", "text": "s ---device.persistence.out", "numinlets": 1, "numoutlets": 0, "patching_rect": [465.0, 75.0, 175.0, 22.0] } },
      { "box": { "id": "hostDiagnostics", "maxclass": "newobj", "text": "print consolidator.devicehost", "numinlets": 1, "numoutlets": 0, "patching_rect": [390.0, 115.0, 175.0, 22.0] } }
    ],
    "lines": [
      { "patchline": { "source": ["busInput", 0], "destination": ["host", 0] } },
      { "patchline": { "source": ["host", 0], "destination": ["busOutput", 0] } },
      { "patchline": { "source": ["persistenceInput", 0], "destination": ["host", 1] } },
      { "patchline": { "source": ["host", 3], "destination": ["persistenceOutput", 0] } },
      { "patchline": { "source": ["host", 1], "destination": ["hostDiagnostics", 0] } },
      { "patchline": { "source": ["host", 2], "destination": ["hostDiagnostics", 0] } }
    ]
  }
}
