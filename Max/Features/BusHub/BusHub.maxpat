{
  "patcher": {
    "fileversion": 1,
    "appversion": {
      "major": 9,
      "minor": 0,
      "revision": 9,
      "architecture": "x64",
      "modernui": 1
    },
    "classnamespace": "box",
    "rect": [100.0, 100.0, 360.0, 180.0],
    "boxes": [
      {
        "box": {
          "id": "busInput",
          "maxclass": "newobj",
          "text": "r ---message.bus.in",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [""],
          "patching_rect": [30.0, 35.0, 145.0, 22.0]
        }
      },
      {
        "box": {
          "id": "busScript",
          "maxclass": "newobj",
          "text": "js BusHub.js",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [""],
          "patching_rect": [110.0, 35.0, 190.0, 22.0],
          "saved_object_attributes": {
            "filename": "BusHub.js",
            "parameter_enable": 0
          }
        }
      },
      {
        "box": {
          "id": "busOutput",
          "maxclass": "newobj",
          "text": "s ---message.bus.out",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [110.0, 105.0, 145.0, 22.0]
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "source": ["busInput", 0],
          "destination": ["busScript", 0]
        }
      },
      {
        "patchline": {
          "source": ["busScript", 0],
          "destination": ["busOutput", 0]
        }
      }
    ]
  }
}
