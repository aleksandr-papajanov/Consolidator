{
    "patcher":  {
                    "fileversion":  1,
                    "appversion":  {
                                       "major":  9,
                                       "minor":  0,
                                       "revision":  9,
                                       "architecture":  "x64",
                                       "modernui":  1
                                   },
                    "classnamespace":  "box",
                    "rect":  [
                                 100.0,
                                 100.0,
                                 760.0,
                                 420.0
                             ],
                    "openinpresentation":  1,
                    "gridsize":  [
                                     10.0,
                                     10.0
                                 ],
                    "boxes":  [
                                  {
                                      "box":  {
                                                  "border":  0,
                                                  "embedstate":  [
                                                                     [
                                                                         "enabled",
                                                                         1
                                                                     ],
                                                                     [
                                                                         "loadingIndex",
                                                                         0
                                                                     ],
                                                                     [
                                                                         "labels",
                                                                         "Match EQ"
                                                                     ],
                                                                     [
                                                                         "selectionMode",
                                                                         "custom"
                                                                     ],
                                                                     [
                                                                         "buttonModes",
                                                                         "momentary"
                                                                     ],
                                                                     [
                                                                         "layout",
                                                                         "horizontal"
                                                                     ],
                                                                     [
                                                                         "allowEmptySelection",
                                                                         1
                                                                     ],
                                                                     [
                                                                         "count",
                                                                         1
                                                                     ]
                                                                 ],
                                                  "filename":  "ButtonGroupControl.js",
                                                  "id":  "match-control",
                                                  "maxclass":  "jsui",
                                                  "numinlets":  1,
                                                  "numoutlets":  1,
                                                  "outlettype":  [
                                                                     ""
                                                                 ],
                                                  "parameter_enable":  0,
                                                  "patching_rect":  [
                                                                        0,
                                                                        0,
                                                                        120,
                                                                        30
                                                                    ],
                                                  "presentation":  1,
                                                  "presentation_rect":  [
                                                                            0,
                                                                            0,
                                                                            120,
                                                                            30
                                                                        ],
                                                  "varname":  "approximator.match"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "border":  0,
                                                  "embedstate":  [
                                                                     [
                                                                         "enabled",
                                                                         1
                                                                     ],
                                                                     [
                                                                         "label",
                                                                         "Listen"
                                                                     ],
                                                                     [
                                                                         "mode",
                                                                         "toggle"
                                                                     ],
                                                                     [
                                                                         "value",
                                                                         0
                                                                     ]
                                                                 ],
                                                  "filename":  "ButtonControl.js",
                                                  "id":  "listen-control",
                                                  "maxclass":  "jsui",
                                                  "numinlets":  1,
                                                  "numoutlets":  1,
                                                  "outlettype":  [
                                                                     ""
                                                                 ],
                                                  "parameter_enable":  0,
                                                  "patching_rect":  [
                                                                        130,
                                                                        0,
                                                                        90,
                                                                        30
                                                                    ],
                                                  "presentation":  1,
                                                  "presentation_rect":  [
                                                                            130,
                                                                            0,
                                                                            90,
                                                                            30
                                                                        ],
                                                  "varname":  "approximator.listen"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "id":  "listen-prepend",
                                                  "maxclass":  "newobj",
                                                  "numinlets":  1,
                                                  "numoutlets":  1,
                                                  "outlettype":  [
                                                                     ""
                                                                 ],
                                                  "patching_rect":  [
                                                                        125.0,
                                                                        70.0,
                                                                        95.0,
                                                                        22.0
                                                                    ],
                                                  "text":  "prepend listen"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "id":  "match-prepend",
                                                  "maxclass":  "newobj",
                                                  "numinlets":  1,
                                                  "numoutlets":  1,
                                                  "outlettype":  [
                                                                     ""
                                                                 ],
                                                  "patching_rect":  [
                                                                        20.0,
                                                                        70.0,
                                                                        95.0,
                                                                        22.0
                                                                    ],
                                                  "text":  "prepend match"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "id":  "controller",
                                                  "maxclass":  "newobj",
                                                  "numinlets":  2,
                                                  "numoutlets":  3,
                                                  "outlettype":  [
                                                                     "",
                                                                     "",
                                                                     ""
                                                                 ],
                                                  "patching_rect":  [
                                                                        20.0,
                                                                        110.0,
                                                                        265.0,
                                                                        22.0
                                                                    ],
                                                  "saved_object_attributes":  {
                                                                                  "filename":  "consolidator.approximator.controller.js",
                                                                                  "parameter_enable":  0
                                                                              },
                                                  "text":  "js consolidator.approximator.controller.js"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "id":  "thispatcher",
                                                  "maxclass":  "newobj",
                                                  "numinlets":  1,
                                                  "numoutlets":  2,
                                                  "outlettype":  [
                                                                     "",
                                                                     ""
                                                                 ],
                                                  "patching_rect":  [
                                                                        300.0,
                                                                        150.0,
                                                                        80.0,
                                                                        22.0
                                                                    ],
                                                  "save":  [
                                                               "#N",
                                                               "thispatcher",
                                                               ";",
                                                               "#Q",
                                                               "end",
                                                               ";"
                                                           ],
                                                  "text":  "thispatcher"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "id":  "bus-receive",
                                                  "maxclass":  "newobj",
                                                  "numinlets":  0,
                                                  "numoutlets":  1,
                                                  "outlettype":  [
                                                                     ""
                                                                 ],
                                                  "patching_rect":  [
                                                                        390.0,
                                                                        70.0,
                                                                        145.0,
                                                                        22.0
                                                                    ],
                                                  "text":  "r ---message.bus.out"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "id":  "controller-bus-send",
                                                  "maxclass":  "newobj",
                                                  "numinlets":  1,
                                                  "numoutlets":  0,
                                                  "patching_rect":  [
                                                                        20.0,
                                                                        150.0,
                                                                        145.0,
                                                                        22.0
                                                                    ],
                                                  "text":  "s ---message.bus.in"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "id":  "native",
                                                  "maxclass":  "newobj",
                                                  "numinlets":  1,
                                                  "numoutlets":  3,
                                                  "outlettype":  [
                                                                     "",
                                                                     "",
                                                                     ""
                                                                 ],
                                                  "patching_rect":  [
                                                                        390.0,
                                                                        190.0,
                                                                        300.0,
                                                                        22.0
                                                                    ],
                                                  "text":  "consolidator.approximator"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "id":  "native-bus-send",
                                                  "maxclass":  "newobj",
                                                  "numinlets":  1,
                                                  "numoutlets":  0,
                                                  "patching_rect":  [
                                                                        390.0,
                                                                        230.0,
                                                                        145.0,
                                                                        22.0
                                                                    ],
                                                  "text":  "s ---message.bus.in"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "id":  "status-print",
                                                  "maxclass":  "newobj",
                                                  "numinlets":  1,
                                                  "numoutlets":  0,
                                                  "patching_rect":  [
                                                                        180.0,
                                                                        150.0,
                                                                        145.0,
                                                                        22.0
                                                                    ],
                                                  "text":  "print approximator.status"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "id":  "debug-print",
                                                  "maxclass":  "newobj",
                                                  "numinlets":  1,
                                                  "numoutlets":  0,
                                                  "patching_rect":  [
                                                                        550.0,
                                                                        230.0,
                                                                        120.0,
                                                                        22.0
                                                                    ],
                                                  "text":  "print approximator"
                                              }
                                  },
                                  {
                                      "box":  {
                                                  "id":  "fit-curve-receive",
                                                  "maxclass":  "newobj",
                                                  "text":  "r ---analyzer.curves",
                                                  "numinlets":  0,
                                                  "numoutlets":  1,
                                                  "outlettype":  [
                                                                     ""
                                                                 ],
                                                  "patching_rect":  [
                                                                        550,
                                                                        70,
                                                                        145,
                                                                        22
                                                                    ]
                                              }
                                  }
                              ],
                    "lines":  [
                                  {
                                      "patchline":  {
                                                        "destination":  [
                                                                            "match-prepend",
                                                                            0
                                                                        ],
                                                        "source":  [
                                                                       "match-control",
                                                                       0
                                                                   ]
                                                    }
                                  },
                                  {
                                      "patchline":  {
                                                        "destination":  [
                                                                            "listen-prepend",
                                                                            0
                                                                        ],
                                                        "source":  [
                                                                       "listen-control",
                                                                       0
                                                                   ]
                                                    }
                                  },
                                  {
                                      "patchline":  {
                                                        "destination":  [
                                                                            "controller",
                                                                            0
                                                                        ],
                                                        "source":  [
                                                                       "listen-prepend",
                                                                       0
                                                                   ]
                                                    }
                                  },
                                  {
                                      "patchline":  {
                                                        "destination":  [
                                                                            "controller",
                                                                            0
                                                                        ],
                                                        "source":  [
                                                                       "match-prepend",
                                                                       0
                                                                   ]
                                                    }
                                  },
                                  {
                                      "patchline":  {
                                                        "destination":  [
                                                                            "controller-bus-send",
                                                                            0
                                                                        ],
                                                        "source":  [
                                                                       "controller",
                                                                       0
                                                                   ]
                                                    }
                                  },
                                  {
                                      "patchline":  {
                                                        "destination":  [
                                                                            "status-print",
                                                                            0
                                                                        ],
                                                        "source":  [
                                                                       "controller",
                                                                       1
                                                                   ]
                                                    }
                                  },
                                  {
                                      "patchline":  {
                                                        "destination":  [
                                                                            "thispatcher",
                                                                            0
                                                                        ],
                                                        "source":  [
                                                                       "controller",
                                                                       2
                                                                   ]
                                                    }
                                  },
                                  {
                                      "patchline":  {
                                                        "destination":  [
                                                                            "native",
                                                                            0
                                                                        ],
                                                        "source":  [
                                                                       "bus-receive",
                                                                       0
                                                                   ]
                                                    }
                                  },
                                  {
                                      "patchline":  {
                                                        "destination":  [
                                                                            "native-bus-send",
                                                                            0
                                                                        ],
                                                        "source":  [
                                                                       "native",
                                                                       0
                                                                   ]
                                                    }
                                  },
                                  {
                                      "patchline":  {
                                                        "destination":  [
                                                                            "controller",
                                                                            1
                                                                        ],
                                                        "source":  [
                                                                       "native",
                                                                       1
                                                                   ]
                                                    }
                                  },
                                  {
                                      "patchline":  {
                                                        "destination":  [
                                                                            "debug-print",
                                                                            0
                                                                        ],
                                                        "source":  [
                                                                       "native",
                                                                       2
                                                                   ]
                                                    }
                                  },
                                  {
                                      "patchline":  {
                                                        "source":  [
                                                                       "fit-curve-receive",
                                                                       0
                                                                   ],
                                                        "destination":  [
                                                                            "controller",
                                                                            1
                                                                        ]
                                                    }
                                  }
                              ],
                    "dependency_cache":  [
                                             {
                                                 "name":  "ButtonControl.js",
                                                 "bootpath":  "D:/Projects/Ableton/Consolidator/Max/Features/Interface",
                                                 "patcherrelativepath":  "../Interface",
                                                 "type":  "TEXT",
                                                 "implicit":  1
                                             },
                                             {
                                                 "name":  "ButtonGroupControl.js",
                                                 "bootpath":  "D:/Projects/Ableton/Consolidator/Max/Features/Interface",
                                                 "patcherrelativepath":  "../Interface",
                                                 "type":  "TEXT",
                                                 "implicit":  1
                                             },
                                             {
                                                 "name":  "consolidator.approximator.controller.js",
                                                 "bootpath":  "D:/Projects/Ableton/Consolidator/Max/Features/Approximator",
                                                 "patcherrelativepath":  ".",
                                                 "type":  "TEXT",
                                                 "implicit":  1
                                             },
                                             {
                                                 "name":  "consolidator.approximator.mxe64",
                                                 "bootpath":  "D:/Projects/Ableton/Consolidator/Max/Features/Approximator",
                                                 "patcherrelativepath":  ".",
                                                 "type":  "mx64",
                                                 "implicit":  1
                                             }
                                         ]
                }
}
