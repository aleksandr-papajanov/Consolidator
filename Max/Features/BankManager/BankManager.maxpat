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
    "rect": [
      100.0,
      100.0,
      584.0,
      412.0
    ],
    "openinpresentation": 1,
    "gridsize": [
      15.0,
      15.0
    ],
    "boxes": [
      {
        "box": {
          "id": "local-receive",
          "maxclass": "newobj",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20.0,
            20.0,
            140.0,
            22.0
          ],
          "text": "r ---message.bus.out"
        }
      },
      {
        "box": {
          "id": "global-receive",
          "maxclass": "newobj",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            180.0,
            20.0,
            140.0,
            22.0
          ],
          "text": "r consolidator.host.bus"
        }
      },
      {
        "box": {
          "filename": "consolidator.bankmanager.js",
          "id": "manager",
          "maxclass": "jsui",
          "numinlets": 2,
          "numoutlets": 3,
          "outlettype": [
            "",
            "",
            ""
          ],
          "parameter_enable": 0,
          "patching_rect": [
            20.0,
            55.0,
            380.0,
            165.0
          ],
          "presentation": 1,
          "presentation_rect": [
            0.0,
            0.0,
            225.0,
            165.0
          ]
        }
      },
      {
        "box": {
          "id": "local-send",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            20.0,
            235.0,
            130.0,
            22.0
          ],
          "text": "s ---message.bus.in"
        }
      },
      {
        "box": {
          "id": "global-send",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            180.0,
            235.0,
            145.0,
            22.0
          ],
          "text": "s consolidator.host.bus"
        }
      },
      {
        "box": {
          "id": "processor-limits-send",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            340.0,
            235.0,
            170.0,
            22.0
          ],
          "text": "s ---processor.link.limits"
        }
      },
      {
        "box": {
          "id": "device",
          "maxclass": "newobj",
          "numinlets": 0,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            20.0,
            270.0,
            90.0,
            22.0
          ],
          "text": "live.thisdevice"
        }
      },
      {
        "box": {
          "id": "initialize-defer",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            120.0,
            270.0,
            60.0,
            22.0
          ],
          "text": "deferlow"
        }
      },
      {
        "box": {
          "id": "initialize",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            190.0,
            270.0,
            60.0,
            22.0
          ],
          "text": "initialize"
        }
      },
      {
        "box": {
          "id": "freebang",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "outlettype": [
            "bang"
          ],
          "patching_rect": [
            260.0,
            300.0,
            65.0,
            22.0
          ],
          "text": "freebang"
        }
      },
      {
        "box": {
          "id": "leave",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            335.0,
            270.0,
            40.0,
            22.0
          ],
          "text": "leave"
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "destination": [
            "manager",
            1
          ],
          "source": [
            "global-receive",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "manager",
            0
          ],
          "source": [
            "local-receive",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "global-send",
            0
          ],
          "source": [
            "manager",
            1
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "local-send",
            0
          ],
          "source": [
            "manager",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "device",
            0
          ],
          "destination": [
            "initialize-defer",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "initialize-defer",
            0
          ],
          "destination": [
            "initialize",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "initialize",
            0
          ],
          "destination": [
            "manager",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "freebang",
            0
          ],
          "destination": [
            "leave",
            0
          ]
        }
      },
      {
        "patchline": {
          "destination": [
            "processor-limits-send",
            0
          ],
          "source": [
            "manager",
            2
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "leave",
            0
          ],
          "destination": [
            "manager",
            0
          ]
        }
      }
    ],
    "saved_attribute_attributes": {
      "default_plcolor": {
        "expression": ""
      }
    },
    "oscreceiveudpport": 0
  }
}
