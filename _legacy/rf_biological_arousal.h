#ifndef RF_BIOLOGICAL_AROUSAL_H
#define RF_BIOLOGICAL_AROUSAL_H

// Target Features: CH_ECG_HR, CH_ECG_Std, CH_ECG_RMSSD, CH_ECG_pNN50, CH_ECG_SDNN, CH_EDA_Mean, CH_EDA_Std, CH_EDA_Min, CH_EDA_Max, CH_EDA_SCL, CH_EDA_SCR, CH_EDA_Peaks

float predict(float* features) {
    float total_score = 0.0f;

    // Tree 0
    float tree_0_score = 0.0f;
        if (features[8] <= 3.0616283416748047f) {
            if (features[4] <= 3.0464941263198853f) {
                if (features[2] <= -2.010030746459961f) {
                    if (features[10] <= 0.252822183072567f) {
                        if (features[0] <= 5.479047179222107f) {
                            tree_0_score = 0.0f;
                        } else {
                            tree_0_score = 1.0f;
                        }
                    } else {
                        tree_0_score = 1.0f;
                    }
                } else {
                    if (features[0] <= 3.0216574668884277f) {
                        if (features[10] <= 3.475401520729065f) {
                            if (features[8] <= 2.8353036642074585f) {
                                tree_0_score = 0.005284791544333529f;
                            } else {
                                tree_0_score = 0.2222222222222222f;
                            }
                        } else {
                            if (features[2] <= -0.40266774594783783f) {
                                tree_0_score = 0.0f;
                            } else {
                                tree_0_score = 1.0f;
                            }
                        }
                    } else {
                        if (features[5] <= 1.632119357585907f) {
                            tree_0_score = 1.0f;
                        } else {
                            if (features[9] <= 1.7197107672691345f) {
                                tree_0_score = 0.0f;
                            } else {
                                tree_0_score = 1.0f;
                            }
                        }
                    }
                }
            } else {
                tree_0_score = 1.0f;
            }
        } else {
            if (features[9] <= 3.996321201324463f) {
                if (features[11] <= -0.1630631871521473f) {
                    if (features[0] <= 0.861101895570755f) {
                        tree_0_score = 0.0f;
                    } else {
                        tree_0_score = 1.0f;
                    }
                } else {
                    tree_0_score = 0.0f;
                }
            } else {
                tree_0_score = 1.0f;
            }
        }
    total_score += tree_0_score;

    // Tree 1
    float tree_1_score = 0.0f;
        if (features[5] <= 3.6108678579330444f) {
            if (features[8] <= 2.0130069255828857f) {
                if (features[10] <= -2.6818524599075317f) {
                    if (features[9] <= 1.9165471196174622f) {
                        tree_1_score = 1.0f;
                    } else {
                        tree_1_score = 0.0f;
                    }
                } else {
                    if (features[3] <= -2.7100155353546143f) {
                        if (features[4] <= -0.3561220169067383f) {
                            tree_1_score = 1.0f;
                        } else {
                            tree_1_score = 0.0f;
                        }
                    } else {
                        if (features[0] <= 3.1392838954925537f) {
                            if (features[3] <= 3.0598714351654053f) {
                                tree_1_score = 0.015921616656460504f;
                            } else {
                                tree_1_score = 0.6153846153846154f;
                            }
                        } else {
                            tree_1_score = 1.0f;
                        }
                    }
                }
            } else {
                if (features[3] <= -2.7797354459762573f) {
                    tree_1_score = 1.0f;
                } else {
                    if (features[1] <= 0.396637424826622f) {
                        tree_1_score = 0.0f;
                    } else {
                        if (features[3] <= -0.43955688178539276f) {
                            tree_1_score = 0.0f;
                        } else {
                            if (features[7] <= 1.7581579089164734f) {
                                tree_1_score = 1.0f;
                            } else {
                                tree_1_score = 0.5454545454545454f;
                            }
                        }
                    }
                }
            }
        } else {
            tree_1_score = 1.0f;
        }
    total_score += tree_1_score;

    // Tree 2
    float tree_2_score = 0.0f;
        if (features[5] <= 3.386462092399597f) {
            if (features[1] <= 0.8710298240184784f) {
                if (features[2] <= -2.028870940208435f) {
                    if (features[7] <= 1.8628039360046387f) {
                        if (features[1] <= -1.0080893337726593f) {
                            if (features[10] <= 0.252822183072567f) {
                                tree_2_score = 0.0f;
                            } else {
                                tree_2_score = 1.0f;
                            }
                        } else {
                            tree_2_score = 1.0f;
                        }
                    } else {
                        tree_2_score = 0.0f;
                    }
                } else {
                    if (features[3] <= -2.3676756620407104f) {
                        if (features[10] <= -2.2875107526779175f) {
                            tree_2_score = 1.0f;
                        } else {
                            tree_2_score = 0.0f;
                        }
                    } else {
                        if (features[5] <= 0.4654018133878708f) {
                            if (features[0] <= 3.1773722171783447f) {
                                tree_2_score = 0.0f;
                            } else {
                                tree_2_score = 1.0f;
                            }
                        } else {
                            if (features[10] <= 5.625105619430542f) {
                                tree_2_score = 0.02977667493796526f;
                            } else {
                                tree_2_score = 1.0f;
                            }
                        }
                    }
                }
            } else {
                if (features[10] <= -2.8324280977249146f) {
                    if (features[0] <= 2.9828078746795654f) {
                        tree_2_score = 0.0f;
                    } else {
                        tree_2_score = 1.0f;
                    }
                } else {
                    if (features[0] <= 2.2284085750579834f) {
                        if (features[11] <= -2.325067400932312f) {
                            if (features[5] <= 0.9628335535526276f) {
                                tree_2_score = 0.0f;
                            } else {
                                tree_2_score = 1.0f;
                            }
                        } else {
                            if (features[1] <= 3.222315788269043f) {
                                tree_2_score = 0.0f;
                            } else {
                                tree_2_score = 0.11538461538461539f;
                            }
                        }
                    } else {
                        if (features[2] <= 0.7377565801143646f) {
                            tree_2_score = 1.0f;
                        } else {
                            if (features[3] <= -0.8685738742351532f) {
                                tree_2_score = 0.4f;
                            } else {
                                tree_2_score = 1.0f;
                            }
                        }
                    }
                }
            }
        } else {
            if (features[8] <= 4.1572136878967285f) {
                if (features[0] <= 0.4312535226345062f) {
                    tree_2_score = 0.0f;
                } else {
                    tree_2_score = 1.0f;
                }
            } else {
                tree_2_score = 1.0f;
            }
        }
    total_score += tree_2_score;

    // Tree 3
    float tree_3_score = 0.0f;
        if (features[5] <= 3.1774851083755493f) {
            if (features[2] <= -2.0554758310317993f) {
                if (features[6] <= -0.5794282555580139f) {
                    tree_3_score = 0.0f;
                } else {
                    if (features[0] <= 0.10067066550254822f) {
                        tree_3_score = 0.0f;
                    } else {
                        tree_3_score = 1.0f;
                    }
                }
            } else {
                if (features[1] <= 0.8613636195659637f) {
                    if (features[7] <= 0.49725043773651123f) {
                        if (features[7] <= -0.8894897401332855f) {
                            if (features[0] <= 3.1864854097366333f) {
                                tree_3_score = 0.0f;
                            } else {
                                tree_3_score = 1.0f;
                            }
                        } else {
                            tree_3_score = 0.0f;
                        }
                    } else {
                        if (features[10] <= 5.65373969078064f) {
                            if (features[0] <= 6.770594477653503f) {
                                tree_3_score = 0.015915119363395226f;
                            } else {
                                tree_3_score = 1.0f;
                            }
                        } else {
                            if (features[9] <= 1.8581457734107971f) {
                                tree_3_score = 0.0f;
                            } else {
                                tree_3_score = 1.0f;
                            }
                        }
                    }
                } else {
                    if (features[0] <= 2.646664023399353f) {
                        if (features[0] <= 1.7714487314224243f) {
                            if (features[5] <= 2.544866442680359f) {
                                tree_3_score = 0.0038461538461538464f;
                            } else {
                                tree_3_score = 0.4f;
                            }
                        } else {
                            if (features[11] <= -1.8279401659965515f) {
                                tree_3_score = 1.0f;
                            } else {
                                tree_3_score = 0.037037037037037035f;
                            }
                        }
                    } else {
                        if (features[9] <= 2.966858386993408f) {
                            if (features[4] <= 2.1795166730880737f) {
                                tree_3_score = 1.0f;
                            } else {
                                tree_3_score = 0.9705882352941176f;
                            }
                        } else {
                            tree_3_score = 0.0f;
                        }
                    }
                }
            }
        } else {
            if (features[7] <= 2.230164408683777f) {
                if (features[1] <= 1.0619613081216812f) {
                    tree_3_score = 0.0f;
                } else {
                    tree_3_score = 1.0f;
                }
            } else {
                if (features[8] <= 3.1341322660446167f) {
                    tree_3_score = 0.0f;
                } else {
                    if (features[9] <= 3.38853919506073f) {
                        if (features[7] <= 3.3511416912078857f) {
                            tree_3_score = 1.0f;
                        } else {
                            tree_3_score = 0.0f;
                        }
                    } else {
                        tree_3_score = 1.0f;
                    }
                }
            }
        }
    total_score += tree_3_score;

    // Tree 4
    float tree_4_score = 0.0f;
        if (features[6] <= 0.9912005960941315f) {
            if (features[8] <= 3.0544053316116333f) {
                if (features[10] <= -2.68441903591156f) {
                    if (features[5] <= 1.7112956643104553f) {
                        tree_4_score = 1.0f;
                    } else {
                        tree_4_score = 0.0f;
                    }
                } else {
                    if (features[0] <= 3.0718895196914673f) {
                        if (features[10] <= 4.248056888580322f) {
                            if (features[2] <= -2.2212579250335693f) {
                                tree_4_score = 0.7368421052631579f;
                            } else {
                                tree_4_score = 0.003486750348675035f;
                            }
                        } else {
                            tree_4_score = 1.0f;
                        }
                    } else {
                        tree_4_score = 1.0f;
                    }
                }
            } else {
                tree_4_score = 1.0f;
            }
        } else {
            if (features[9] <= 3.6165958642959595f) {
                if (features[0] <= 2.5647456645965576f) {
                    if (features[10] <= 3.526843547821045f) {
                        if (features[4] <= -1.508484125137329f) {
                            if (features[10] <= 1.159838855266571f) {
                                tree_4_score = 0.875f;
                            } else {
                                tree_4_score = 0.0f;
                            }
                        } else {
                            if (features[2] <= -1.7515620589256287f) {
                                tree_4_score = 1.0f;
                            } else {
                                tree_4_score = 0.014705882352941176f;
                            }
                        }
                    } else {
                        if (features[10] <= 8.076039791107178f) {
                            tree_4_score = 1.0f;
                        } else {
                            tree_4_score = 0.0f;
                        }
                    }
                } else {
                    if (features[11] <= -0.4684758484363556f) {
                        if (features[5] <= 1.6115905046463013f) {
                            tree_4_score = 1.0f;
                        } else {
                            if (features[9] <= 1.6289429664611816f) {
                                tree_4_score = 0.0f;
                            } else {
                                tree_4_score = 0.9795918367346939f;
                            }
                        }
                    } else {
                        if (features[3] <= -1.1757596731185913f) {
                            if (features[2] <= -1.2139878869056702f) {
                                tree_4_score = 0.7777777777777778f;
                            } else {
                                tree_4_score = 1.0f;
                            }
                        } else {
                            tree_4_score = 0.0f;
                        }
                    }
                }
            } else {
                tree_4_score = 1.0f;
            }
        }
    total_score += tree_4_score;

    // Tree 5
    float tree_5_score = 0.0f;
        if (features[7] <= 2.2768865823745728f) {
            if (features[11] <= -1.851509690284729f) {
                if (features[9] <= 1.0565522909164429f) {
                    if (features[6] <= 0.8324102386832237f) {
                        if (features[1] <= 1.9008274674415588f) {
                            tree_5_score = 0.0f;
                        } else {
                            tree_5_score = 1.0f;
                        }
                    } else {
                        tree_5_score = 1.0f;
                    }
                } else {
                    if (features[3] <= -1.7174227982759476f) {
                        tree_5_score = 1.0f;
                    } else {
                        if (features[2] <= -0.4660330265760422f) {
                            tree_5_score = 0.0f;
                        } else {
                            tree_5_score = 1.0f;
                        }
                    }
                }
            } else {
                if (features[4] <= 3.035856604576111f) {
                    if (features[1] <= 2.5411875247955322f) {
                        if (features[6] <= 6.053905010223389f) {
                            if (features[10] <= -2.624568462371826f) {
                                tree_5_score = 0.9722222222222222f;
                            } else {
                                tree_5_score = 0.025308641975308643f;
                            }
                        } else {
                            if (features[11] <= -1.2931588888168335f) {
                                tree_5_score = 0.0f;
                            } else {
                                tree_5_score = 1.0f;
                            }
                        }
                    } else {
                        if (features[0] <= 3.8492270708084106f) {
                            if (features[9] <= -0.5396738946437836f) {
                                tree_5_score = 0.6f;
                            } else {
                                tree_5_score = 0.0f;
                            }
                        } else {
                            tree_5_score = 1.0f;
                        }
                    }
                } else {
                    if (features[4] <= 3.0632457733154297f) {
                        if (features[1] <= 4.158513307571411f) {
                            tree_5_score = 0.0f;
                        } else {
                            tree_5_score = 1.0f;
                        }
                    } else {
                        tree_5_score = 1.0f;
                    }
                }
            }
        } else {
            if (features[8] <= 3.0678120851516724f) {
                if (features[0] <= 1.7385590076446533f) {
                    if (features[5] <= 2.6091606616973877f) {
                        if (features[10] <= 3.028676986694336f) {
                            if (features[9] <= 2.562943458557129f) {
                                tree_5_score = 0.0f;
                            } else {
                                tree_5_score = 1.0f;
                            }
                        } else {
                            tree_5_score = 1.0f;
                        }
                    } else {
                        tree_5_score = 0.0f;
                    }
                } else {
                    tree_5_score = 1.0f;
                }
            } else {
                if (features[7] <= 3.5108546018600464f) {
                    if (features[1] <= 0.8098268806934357f) {
                        tree_5_score = 0.0f;
                    } else {
                        if (features[8] <= 3.239470601081848f) {
                            if (features[3] <= 0.11147937644273043f) {
                                tree_5_score = 1.0f;
                            } else {
                                tree_5_score = 0.0f;
                            }
                        } else {
                            tree_5_score = 1.0f;
                        }
                    }
                } else {
                    tree_5_score = 1.0f;
                }
            }
        }
    total_score += tree_5_score;

    // Tree 6
    float tree_6_score = 0.0f;
        if (features[7] <= 2.269965648651123f) {
            if (features[1] <= 2.252519369125366f) {
                if (features[3] <= -2.1327807903289795f) {
                    if (features[6] <= -0.1540069542825222f) {
                        if (features[5] <= -0.5292415022850037f) {
                            tree_6_score = 0.0f;
                        } else {
                            tree_6_score = 1.0f;
                        }
                    } else {
                        if (features[5] <= 0.8947262167930603f) {
                            tree_6_score = 1.0f;
                        } else {
                            if (features[4] <= -0.6404927372932434f) {
                                tree_6_score = 1.0f;
                            } else {
                                tree_6_score = 0.0f;
                            }
                        }
                    }
                } else {
                    if (features[8] <= 2.0130069255828857f) {
                        if (features[5] <= -1.930125117301941f) {
                            if (features[7] <= -2.0848864316940308f) {
                                tree_6_score = 0.0f;
                            } else {
                                tree_6_score = 1.0f;
                            }
                        } else {
                            if (features[4] <= 2.9747116565704346f) {
                                tree_6_score = 0.018147684605757195f;
                            } else {
                                tree_6_score = 1.0f;
                            }
                        }
                    } else {
                        if (features[0] <= 1.628899872303009f) {
                            tree_6_score = 0.0f;
                        } else {
                            tree_6_score = 1.0f;
                        }
                    }
                }
            } else {
                if (features[0] <= 3.641792416572571f) {
                    if (features[9] <= -0.5388480424880981f) {
                        if (features[0] <= 0.31224092841148376f) {
                            tree_6_score = 0.0f;
                        } else {
                            if (features[5] <= -0.6012191772460938f) {
                                tree_6_score = 0.0f;
                            } else {
                                tree_6_score = 1.0f;
                            }
                        }
                    } else {
                        tree_6_score = 0.0f;
                    }
                } else {
                    tree_6_score = 1.0f;
                }
            }
        } else {
            if (features[8] <= 3.0910874605178833f) {
                if (features[0] <= 2.7520196437835693f) {
                    if (features[1] <= 0.18446405231952667f) {
                        tree_6_score = 0.0f;
                    } else {
                        if (features[11] <= -1.8128557801246643f) {
                            tree_6_score = 1.0f;
                        } else {
                            tree_6_score = 0.0f;
                        }
                    }
                } else {
                    tree_6_score = 1.0f;
                }
            } else {
                if (features[0] <= 4.418015956878662f) {
                    if (features[9] <= 3.2799410820007324f) {
                        if (features[2] <= 2.0696906447410583f) {
                            tree_6_score = 0.0f;
                        } else {
                            tree_6_score = 1.0f;
                        }
                    } else {
                        tree_6_score = 1.0f;
                    }
                } else {
                    tree_6_score = 1.0f;
                }
            }
        }
    total_score += tree_6_score;

    // Tree 7
    float tree_7_score = 0.0f;
        if (features[0] <= 2.4454246759414673f) {
            if (features[9] <= 3.854978561401367f) {
                if (features[2] <= -2.0805742740631104f) {
                    if (features[7] <= 1.165748417377472f) {
                        if (features[7] <= 0.3863425888121128f) {
                            tree_7_score = 1.0f;
                        } else {
                            if (features[9] <= 0.5963193625211716f) {
                                tree_7_score = 0.0f;
                            } else {
                                tree_7_score = 1.0f;
                            }
                        }
                    } else {
                        tree_7_score = 0.0f;
                    }
                } else {
                    if (features[4] <= 3.1842774152755737f) {
                        if (features[10] <= 3.8226184844970703f) {
                            if (features[11] <= -2.375714898109436f) {
                                tree_7_score = 0.16666666666666666f;
                            } else {
                                tree_7_score = 0.005447941888619854f;
                            }
                        } else {
                            if (features[7] <= 0.9631990790367126f) {
                                tree_7_score = 0.0f;
                            } else {
                                tree_7_score = 1.0f;
                            }
                        }
                    } else {
                        tree_7_score = 1.0f;
                    }
                }
            } else {
                tree_7_score = 1.0f;
            }
        } else {
            if (features[0] <= 2.6915591955184937f) {
                if (features[8] <= 1.617189347743988f) {
                    tree_7_score = 0.0f;
                } else {
                    if (features[11] <= -0.8299315869808197f) {
                        tree_7_score = 1.0f;
                    } else {
                        tree_7_score = 0.0f;
                    }
                }
            } else {
                if (features[0] <= 3.431280255317688f) {
                    if (features[0] <= 3.403740882873535f) {
                        if (features[9] <= 1.6289429664611816f) {
                            if (features[10] <= -1.1011359691619873f) {
                                tree_7_score = 1.0f;
                            } else {
                                tree_7_score = 0.0f;
                            }
                        } else {
                            tree_7_score = 1.0f;
                        }
                    } else {
                        tree_7_score = 0.0f;
                    }
                } else {
                    tree_7_score = 1.0f;
                }
            }
        }
    total_score += tree_7_score;

    // Tree 8
    float tree_8_score = 0.0f;
        if (features[8] <= 3.061324715614319f) {
            if (features[0] <= 3.0555726289749146f) {
                if (features[8] <= 1.9643176794052124f) {
                    if (features[10] <= 5.738806247711182f) {
                        if (features[0] <= 0.6845654249191284f) {
                            if (features[1] <= 3.056986689567566f) {
                                tree_8_score = 0.0f;
                            } else {
                                tree_8_score = 0.07692307692307693f;
                            }
                        } else {
                            if (features[3] <= -3.2942618131637573f) {
                                tree_8_score = 1.0f;
                            } else {
                                tree_8_score = 0.0339943342776204f;
                            }
                        }
                    } else {
                        tree_8_score = 1.0f;
                    }
                } else {
                    if (features[10] <= 3.526843547821045f) {
                        if (features[7] <= 2.6211835145950317f) {
                            tree_8_score = 0.0f;
                        } else {
                            if (features[7] <= 2.7009952068328857f) {
                                tree_8_score = 0.6f;
                            } else {
                                tree_8_score = 0.0f;
                            }
                        }
                    } else {
                        tree_8_score = 1.0f;
                    }
                }
            } else {
                if (features[10] <= 1.7520326375961304f) {
                    tree_8_score = 1.0f;
                } else {
                    if (features[11] <= -1.6784006357192993f) {
                        tree_8_score = 1.0f;
                    } else {
                        tree_8_score = 0.0f;
                    }
                }
            }
        } else {
            if (features[5] <= 3.2627859115600586f) {
                if (features[10] <= 2.94103741645813f) {
                    if (features[6] <= 3.375910997390747f) {
                        tree_8_score = 0.0f;
                    } else {
                        tree_8_score = 1.0f;
                    }
                } else {
                    tree_8_score = 1.0f;
                }
            } else {
                tree_8_score = 1.0f;
            }
        }
    total_score += tree_8_score;

    // Tree 9
    float tree_9_score = 0.0f;
        if (features[9] <= 2.880535840988159f) {
            if (features[6] <= 1.0755857229232788f) {
                if (features[4] <= 3.049095630645752f) {
                    if (features[0] <= 3.0216574668884277f) {
                        if (features[2] <= -2.0805742740631104f) {
                            if (features[3] <= -2.286898136138916f) {
                                tree_9_score = 1.0f;
                            } else {
                                tree_9_score = 0.0f;
                            }
                        } else {
                            if (features[10] <= 3.7711764574050903f) {
                                tree_9_score = 0.004067796610169492f;
                            } else {
                                tree_9_score = 1.0f;
                            }
                        }
                    } else {
                        tree_9_score = 1.0f;
                    }
                } else {
                    tree_9_score = 1.0f;
                }
            } else {
                if (features[0] <= 2.4201738834381104f) {
                    if (features[4] <= -1.0960622429847717f) {
                        if (features[2] <= -1.8480432033538818f) {
                            tree_9_score = 1.0f;
                        } else {
                            if (features[3] <= -1.3411808013916016f) {
                                tree_9_score = 0.3333333333333333f;
                            } else {
                                tree_9_score = 0.0f;
                            }
                        }
                    } else {
                        if (features[0] <= 1.2112972140312195f) {
                            if (features[7] <= 0.16324717551469803f) {
                                tree_9_score = 0.03571428571428571f;
                            } else {
                                tree_9_score = 0.0f;
                            }
                        } else {
                            if (features[11] <= -2.102652609348297f) {
                                tree_9_score = 1.0f;
                            } else {
                                tree_9_score = 0.13513513513513514f;
                            }
                        }
                    }
                } else {
                    if (features[1] <= 0.6220076382160187f) {
                        if (features[8] <= 0.3956954777240753f) {
                            tree_9_score = 0.0f;
                        } else {
                            tree_9_score = 1.0f;
                        }
                    } else {
                        if (features[4] <= 2.1795166730880737f) {
                            if (features[3] <= -1.1078458428382874f) {
                                tree_9_score = 1.0f;
                            } else {
                                tree_9_score = 0.9473684210526315f;
                            }
                        } else {
                            if (features[7] <= 0.7383949682116508f) {
                                tree_9_score = 1.0f;
                            } else {
                                tree_9_score = 0.0f;
                            }
                        }
                    }
                }
            }
        } else {
            if (features[8] <= 3.0678120851516724f) {
                tree_9_score = 0.0f;
            } else {
                if (features[0] <= -2.2614084482192993f) {
                    tree_9_score = 0.0f;
                } else {
                    if (features[0] <= -0.891922265291214f) {
                        if (features[7] <= 5.858077645301819f) {
                            tree_9_score = 0.0f;
                        } else {
                            tree_9_score = 1.0f;
                        }
                    } else {
                        if (features[11] <= 0.8414037227630615f) {
                            tree_9_score = 1.0f;
                        } else {
                            if (features[11] <= 0.8485283851623535f) {
                                tree_9_score = 0.0f;
                            } else {
                                tree_9_score = 1.0f;
                            }
                        }
                    }
                }
            }
        }
    total_score += tree_9_score;

    // Tree 10
    float tree_10_score = 0.0f;
        if (features[0] <= 2.705164909362793f) {
            if (features[7] <= 3.6428146362304688f) {
                if (features[2] <= -2.149408459663391f) {
                    if (features[8] <= 1.1281943619251251f) {
                        tree_10_score = 1.0f;
                    } else {
                        tree_10_score = 0.0f;
                    }
                } else {
                    if (features[3] <= 3.0344743728637695f) {
                        if (features[10] <= 3.526843547821045f) {
                            if (features[8] <= 2.8353036642074585f) {
                                tree_10_score = 0.007738095238095238f;
                            } else {
                                tree_10_score = 0.21052631578947367f;
                            }
                        } else {
                            if (features[1] <= 0.12014288082718849f) {
                                tree_10_score = 0.0f;
                            } else {
                                tree_10_score = 1.0f;
                            }
                        }
                    } else {
                        if (features[11] <= -0.811311662197113f) {
                            tree_10_score = 1.0f;
                        } else {
                            tree_10_score = 0.0f;
                        }
                    }
                }
            } else {
                tree_10_score = 1.0f;
            }
        } else {
            if (features[6] <= -0.6214973330497742f) {
                if (features[3] <= -1.9982123970985413f) {
                    tree_10_score = 0.0f;
                } else {
                    tree_10_score = 1.0f;
                }
            } else {
                tree_10_score = 1.0f;
            }
        }
    total_score += tree_10_score;

    // Tree 11
    float tree_11_score = 0.0f;
        if (features[8] <= 3.0678120851516724f) {
            if (features[0] <= 3.0216574668884277f) {
                if (features[2] <= -2.1757864952087402f) {
                    if (features[10] <= 0.3651593178510666f) {
                        tree_11_score = 0.0f;
                    } else {
                        tree_11_score = 1.0f;
                    }
                } else {
                    if (features[0] <= 1.6851906776428223f) {
                        if (features[4] <= 3.1842774152755737f) {
                            if (features[7] <= 2.6211835145950317f) {
                                tree_11_score = 0.00314070351758794f;
                            } else {
                                tree_11_score = 0.17391304347826086f;
                            }
                        } else {
                            tree_11_score = 1.0f;
                        }
                    } else {
                        if (features[11] <= -2.239949941635132f) {
                            tree_11_score = 1.0f;
                        } else {
                            if (features[2] <= 2.4357200860977173f) {
                                tree_11_score = 0.0136986301369863f;
                            } else {
                                tree_11_score = 1.0f;
                            }
                        }
                    }
                }
            } else {
                if (features[0] <= 3.3145676851272583f) {
                    if (features[2] <= 1.047254347591661f) {
                        tree_11_score = 1.0f;
                    } else {
                        tree_11_score = 0.0f;
                    }
                } else {
                    tree_11_score = 1.0f;
                }
            }
        } else {
            if (features[1] <= -0.15570232272148132f) {
                if (features[8] <= 5.759661436080933f) {
                    tree_11_score = 0.0f;
                } else {
                    tree_11_score = 1.0f;
                }
            } else {
                tree_11_score = 1.0f;
            }
        }
    total_score += tree_11_score;

    // Tree 12
    float tree_12_score = 0.0f;
        if (features[9] <= 3.1164575815200806f) {
            if (features[8] <= 2.0130069255828857f) {
                if (features[10] <= -2.5762438774108887f) {
                    if (features[3] <= 0.7915429174900055f) {
                        tree_12_score = 1.0f;
                    } else {
                        if (features[7] <= -0.6498569399118423f) {
                            tree_12_score = 1.0f;
                        } else {
                            tree_12_score = 0.0f;
                        }
                    }
                } else {
                    if (features[0] <= 3.0216574668884277f) {
                        if (features[2] <= -2.0805742740631104f) {
                            if (features[1] <= -1.0080893337726593f) {
                                tree_12_score = 0.3333333333333333f;
                            } else {
                                tree_12_score = 0.9642857142857143f;
                            }
                        } else {
                            if (features[10] <= 3.4539400339126587f) {
                                tree_12_score = 0.006781750924784217f;
                            } else {
                                tree_12_score = 0.75f;
                            }
                        }
                    } else {
                        tree_12_score = 1.0f;
                    }
                }
            } else {
                if (features[5] <= 2.4633949995040894f) {
                    if (features[11] <= -1.719784438610077f) {
                        if (features[0] <= 0.9656955301761627f) {
                            tree_12_score = 0.0f;
                        } else {
                            tree_12_score = 1.0f;
                        }
                    } else {
                        if (features[7] <= 2.4555728435516357f) {
                            tree_12_score = 0.0f;
                        } else {
                            if (features[5] <= 2.413847804069519f) {
                                tree_12_score = 0.0f;
                            } else {
                                tree_12_score = 1.0f;
                            }
                        }
                    }
                } else {
                    if (features[1] <= 1.7460873126983643f) {
                        if (features[0] <= 1.3814011812210083f) {
                            tree_12_score = 0.0f;
                        } else {
                            if (features[7] <= 2.6915221214294434f) {
                                tree_12_score = 1.0f;
                            } else {
                                tree_12_score = 0.0f;
                            }
                        }
                    } else {
                        if (features[9] <= 2.966858386993408f) {
                            tree_12_score = 1.0f;
                        } else {
                            tree_12_score = 0.0f;
                        }
                    }
                }
            }
        } else {
            if (features[9] <= 3.627493143081665f) {
                if (features[8] <= 3.211721181869507f) {
                    tree_12_score = 0.0f;
                } else {
                    if (features[5] <= 3.595782160758972f) {
                        tree_12_score = 1.0f;
                    } else {
                        tree_12_score = 0.0f;
                    }
                }
            } else {
                if (features[8] <= 4.097318172454834f) {
                    if (features[1] <= 0.002802908420562744f) {
                        tree_12_score = 0.0f;
                    } else {
                        tree_12_score = 1.0f;
                    }
                } else {
                    tree_12_score = 1.0f;
                }
            }
        }
    total_score += tree_12_score;

    // Tree 13
    float tree_13_score = 0.0f;
        if (features[8] <= 3.0678120851516724f) {
            if (features[11] <= -2.235975980758667f) {
                if (features[4] <= -0.10816159471869469f) {
                    if (features[3] <= 0.9281979203224182f) {
                        if (features[0] <= 0.7055684849619865f) {
                            tree_13_score = 0.0f;
                        } else {
                            tree_13_score = 1.0f;
                        }
                    } else {
                        tree_13_score = 1.0f;
                    }
                } else {
                    if (features[0] <= 0.9227895438671112f) {
                        tree_13_score = 0.0f;
                    } else {
                        tree_13_score = 1.0f;
                    }
                }
            } else {
                if (features[10] <= -2.732178568840027f) {
                    if (features[0] <= 3.2393813133239746f) {
                        tree_13_score = 0.0f;
                    } else {
                        tree_13_score = 1.0f;
                    }
                } else {
                    if (features[10] <= 8.651193141937256f) {
                        if (features[4] <= 3.049095630645752f) {
                            if (features[0] <= 3.0216574668884277f) {
                                tree_13_score = 0.01622596153846154f;
                            } else {
                                tree_13_score = 1.0f;
                            }
                        } else {
                            tree_13_score = 1.0f;
                        }
                    } else {
                        tree_13_score = 1.0f;
                    }
                }
            }
        } else {
            if (features[9] <= 3.3510093688964844f) {
                if (features[0] <= 1.1785861030220985f) {
                    tree_13_score = 0.0f;
                } else {
                    tree_13_score = 1.0f;
                }
            } else {
                if (features[0] <= -0.6964557468891144f) {
                    if (features[3] <= 1.0878275036811829f) {
                        tree_13_score = 0.0f;
                    } else {
                        tree_13_score = 1.0f;
                    }
                } else {
                    tree_13_score = 1.0f;
                }
            }
        }
    total_score += tree_13_score;

    // Tree 14
    float tree_14_score = 0.0f;
        if (features[7] <= 2.2840346097946167f) {
            if (features[3] <= -2.3777780532836914f) {
                if (features[0] <= 4.01761257648468f) {
                    if (features[0] <= 1.9553872346878052f) {
                        if (features[8] <= 1.505622386932373f) {
                            tree_14_score = 1.0f;
                        } else {
                            tree_14_score = 0.0f;
                        }
                    } else {
                        tree_14_score = 0.0f;
                    }
                } else {
                    tree_14_score = 1.0f;
                }
            } else {
                if (features[1] <= 0.9501712322235107f) {
                    if (features[10] <= 8.81649112701416f) {
                        if (features[2] <= -2.0554758310317993f) {
                            if (features[6] <= 0.7287258505821228f) {
                                tree_14_score = 0.0f;
                            } else {
                                tree_14_score = 1.0f;
                            }
                        } else {
                            if (features[3] <= -0.849554181098938f) {
                                tree_14_score = 0.056140350877192984f;
                            } else {
                                tree_14_score = 0.0f;
                            }
                        }
                    } else {
                        tree_14_score = 1.0f;
                    }
                } else {
                    if (features[3] <= -0.912849634885788f) {
                        if (features[6] <= -0.42960961163043976f) {
                            if (features[6] <= -0.6985751688480377f) {
                                tree_14_score = 1.0f;
                            } else {
                                tree_14_score = 0.0f;
                            }
                        } else {
                            if (features[5] <= 0.7285059988498688f) {
                                tree_14_score = 0.8837209302325582f;
                            } else {
                                tree_14_score = 0.4090909090909091f;
                            }
                        }
                    } else {
                        if (features[2] <= 3.34040629863739f) {
                            if (features[8] <= 1.9631497263908386f) {
                                tree_14_score = 0.13043478260869565f;
                            } else {
                                tree_14_score = 0.8888888888888888f;
                            }
                        } else {
                            if (features[6] <= -0.07416468393057585f) {
                                tree_14_score = 0.15384615384615385f;
                            } else {
                                tree_14_score = 0.9655172413793104f;
                            }
                        }
                    }
                }
            }
        } else {
            if (features[5] <= 3.1786718368530273f) {
                if (features[10] <= 4.323784828186035f) {
                    if (features[0] <= 1.1320261359214783f) {
                        tree_14_score = 0.0f;
                    } else {
                        if (features[4] <= 2.318492889404297f) {
                            if (features[11] <= -0.901438295841217f) {
                                tree_14_score = 1.0f;
                            } else {
                                tree_14_score = 0.0f;
                            }
                        } else {
                            tree_14_score = 0.0f;
                        }
                    }
                } else {
                    tree_14_score = 1.0f;
                }
            } else {
                if (features[8] <= 3.1341322660446167f) {
                    tree_14_score = 0.0f;
                } else {
                    if (features[9] <= 4.0142199993133545f) {
                        if (features[5] <= 3.958757162094116f) {
                            if (features[8] <= 4.011933326721191f) {
                                tree_14_score = 1.0f;
                            } else {
                                tree_14_score = 0.0f;
                            }
                        } else {
                            tree_14_score = 0.0f;
                        }
                    } else {
                        tree_14_score = 1.0f;
                    }
                }
            }
        }
    total_score += tree_14_score;

    // Tree 15
    float tree_15_score = 0.0f;
        if (features[7] <= 2.269965648651123f) {
            if (features[4] <= 3.0221517086029053f) {
                if (features[0] <= 3.3205446004867554f) {
                    if (features[2] <= -2.0554758310317993f) {
                        if (features[0] <= 0.10983496904373169f) {
                            tree_15_score = 0.0f;
                        } else {
                            tree_15_score = 1.0f;
                        }
                    } else {
                        if (features[11] <= -2.3453528881073f) {
                            if (features[4] <= 0.16980739124119282f) {
                                tree_15_score = 0.0f;
                            } else {
                                tree_15_score = 0.8461538461538461f;
                            }
                        } else {
                            if (features[11] <= -1.910916268825531f) {
                                tree_15_score = 0.058823529411764705f;
                            } else {
                                tree_15_score = 0.005009392611145898f;
                            }
                        }
                    }
                } else {
                    if (features[2] <= 2.0491749048233032f) {
                        tree_15_score = 1.0f;
                    } else {
                        if (features[9] <= 1.3243074417114258f) {
                            tree_15_score = 1.0f;
                        } else {
                            tree_15_score = 0.0f;
                        }
                    }
                }
            } else {
                tree_15_score = 1.0f;
            }
        } else {
            if (features[9] <= 3.3510093688964844f) {
                if (features[9] <= 2.6046382188796997f) {
                    if (features[0] <= 1.368226945400238f) {
                        if (features[10] <= 3.028676986694336f) {
                            tree_15_score = 0.0f;
                        } else {
                            tree_15_score = 1.0f;
                        }
                    } else {
                        tree_15_score = 1.0f;
                    }
                } else {
                    if (features[11] <= -1.8200436234474182f) {
                        if (features[11] <= -2.692670226097107f) {
                            tree_15_score = 0.0f;
                        } else {
                            tree_15_score = 1.0f;
                        }
                    } else {
                        if (features[3] <= -3.8035278916358948f) {
                            tree_15_score = 1.0f;
                        } else {
                            tree_15_score = 0.0f;
                        }
                    }
                }
            } else {
                tree_15_score = 1.0f;
            }
        }
    total_score += tree_15_score;

    // Tree 16
    float tree_16_score = 0.0f;
        if (features[9] <= 3.079315423965454f) {
            if (features[6] <= 1.0072101354599f) {
                if (features[0] <= 2.8698203563690186f) {
                    if (features[3] <= -2.42059326171875f) {
                        if (features[7] <= -0.5818532407283783f) {
                            tree_16_score = 0.0f;
                        } else {
                            tree_16_score = 1.0f;
                        }
                    } else {
                        if (features[3] <= 3.0598714351654053f) {
                            if (features[1] <= 3.0636526346206665f) {
                                tree_16_score = 0.0035587188612099642f;
                            } else {
                                tree_16_score = 0.12f;
                            }
                        } else {
                            if (features[9] <= 1.2526214718818665f) {
                                tree_16_score = 0.0f;
                            } else {
                                tree_16_score = 1.0f;
                            }
                        }
                    }
                } else {
                    if (features[0] <= 3.1864854097366333f) {
                        if (features[0] <= 2.9568973779678345f) {
                            tree_16_score = 1.0f;
                        } else {
                            tree_16_score = 0.0f;
                        }
                    } else {
                        tree_16_score = 1.0f;
                    }
                }
            } else {
                if (features[10] <= -2.7720435857772827f) {
                    if (features[7] <= 2.3467347621917725f) {
                        tree_16_score = 1.0f;
                    } else {
                        tree_16_score = 0.0f;
                    }
                } else {
                    if (features[3] <= -2.3715070486068726f) {
                        if (features[6] <= 2.56576931476593f) {
                            tree_16_score = 1.0f;
                        } else {
                            if (features[6] <= 2.7629902362823486f) {
                                tree_16_score = 0.0f;
                            } else {
                                tree_16_score = 0.8888888888888888f;
                            }
                        }
                    } else {
                        if (features[11] <= -1.807516634464264f) {
                            if (features[4] <= -0.395735964179039f) {
                                tree_16_score = 0.0f;
                            } else {
                                tree_16_score = 0.9393939393939394f;
                            }
                        } else {
                            if (features[9] <= 0.8490707278251648f) {
                                tree_16_score = 0.12f;
                            } else {
                                tree_16_score = 0.0f;
                            }
                        }
                    }
                }
            }
        } else {
            if (features[7] <= 3.5108546018600464f) {
                if (features[0] <= 1.781384915113449f) {
                    tree_16_score = 0.0f;
                } else {
                    tree_16_score = 1.0f;
                }
            } else {
                tree_16_score = 1.0f;
            }
        }
    total_score += tree_16_score;

    // Tree 17
    float tree_17_score = 0.0f;
        if (features[9] <= 3.383841872215271f) {
            if (features[3] <= -2.5368428230285645f) {
                if (features[0] <= 3.725825071334839f) {
                    if (features[2] <= -1.9050323963165283f) {
                        tree_17_score = 1.0f;
                    } else {
                        tree_17_score = 0.0f;
                    }
                } else {
                    tree_17_score = 1.0f;
                }
            } else {
                if (features[10] <= -2.784183979034424f) {
                    if (features[1] <= 0.28906582295894623f) {
                        tree_17_score = 0.0f;
                    } else {
                        if (features[9] <= 1.9502636790275574f) {
                            tree_17_score = 1.0f;
                        } else {
                            tree_17_score = 0.0f;
                        }
                    }
                } else {
                    if (features[2] <= 4.872177839279175f) {
                        if (features[10] <= 5.738806247711182f) {
                            if (features[0] <= 2.8692221641540527f) {
                                tree_17_score = 0.013569321533923304f;
                            } else {
                                tree_17_score = 0.8181818181818182f;
                            }
                        } else {
                            if (features[2] <= -0.9605163335800171f) {
                                tree_17_score = 0.0f;
                            } else {
                                tree_17_score = 1.0f;
                            }
                        }
                    } else {
                        tree_17_score = 1.0f;
                    }
                }
            }
        } else {
            if (features[0] <= -0.7059133946895599f) {
                if (features[11] <= -1.7944416403770447f) {
                    tree_17_score = 0.0f;
                } else {
                    tree_17_score = 1.0f;
                }
            } else {
                tree_17_score = 1.0f;
            }
        }
    total_score += tree_17_score;

    // Tree 18
    float tree_18_score = 0.0f;
        if (features[1] <= 0.885991245508194f) {
            if (features[7] <= 3.5135124921798706f) {
                if (features[2] <= -2.29419481754303f) {
                    if (features[10] <= 0.3452425003051758f) {
                        if (features[5] <= 1.170729547739029f) {
                            tree_18_score = 0.0f;
                        } else {
                            tree_18_score = 1.0f;
                        }
                    } else {
                        tree_18_score = 1.0f;
                    }
                } else {
                    if (features[11] <= -2.2374075651168823f) {
                        if (features[0] <= 0.7806608378887177f) {
                            tree_18_score = 0.0f;
                        } else {
                            tree_18_score = 1.0f;
                        }
                    } else {
                        if (features[10] <= -3.0305631160736084f) {
                            tree_18_score = 1.0f;
                        } else {
                            if (features[0] <= 3.1864854097366333f) {
                                tree_18_score = 0.0064516129032258064f;
                            } else {
                                tree_18_score = 1.0f;
                            }
                        }
                    }
                }
            } else {
                tree_18_score = 1.0f;
            }
        } else {
            if (features[10] <= -1.9306011199951172f) {
                if (features[0] <= 1.3614044785499573f) {
                    if (features[5] <= 17.715192317962646f) {
                        tree_18_score = 0.0f;
                    } else {
                        tree_18_score = 1.0f;
                    }
                } else {
                    if (features[11] <= 0.5431585609912872f) {
                        tree_18_score = 1.0f;
                    } else {
                        if (features[3] <= 0.167796328663826f) {
                            tree_18_score = 1.0f;
                        } else {
                            if (features[9] <= 24.902206420898438f) {
                                tree_18_score = 0.0f;
                            } else {
                                tree_18_score = 1.0f;
                            }
                        }
                    }
                }
            } else {
                if (features[0] <= 2.451119542121887f) {
                    if (features[6] <= 3.2606847286224365f) {
                        if (features[9] <= 3.7533448934555054f) {
                            if (features[7] <= 1.5366005301475525f) {
                                tree_18_score = 0.01593625498007968f;
                            } else {
                                tree_18_score = 0.3125f;
                            }
                        } else {
                            tree_18_score = 1.0f;
                        }
                    } else {
                        if (features[5] <= 1.9057015180587769f) {
                            tree_18_score = 0.0f;
                        } else {
                            if (features[8] <= 2.9983311891555786f) {
                                tree_18_score = 0.75f;
                            } else {
                                tree_18_score = 1.0f;
                            }
                        }
                    }
                } else {
                    if (features[7] <= 1.3412263989448547f) {
                        if (features[0] <= 3.673690676689148f) {
                            tree_18_score = 0.0f;
                        } else {
                            tree_18_score = 1.0f;
                        }
                    } else {
                        tree_18_score = 1.0f;
                    }
                }
            }
        }
    total_score += tree_18_score;

    // Tree 19
    float tree_19_score = 0.0f;
        if (features[8] <= 3.0678120851516724f) {
            if (features[8] <= 2.147881507873535f) {
                if (features[4] <= 3.049095630645752f) {
                    if (features[2] <= -2.0805742740631104f) {
                        if (features[11] <= 0.9805342555046082f) {
                            tree_19_score = 1.0f;
                        } else {
                            tree_19_score = 0.0f;
                        }
                    } else {
                        if (features[1] <= 0.9986428022384644f) {
                            if (features[9] <= 0.4555277079343796f) {
                                tree_19_score = 0.003683241252302026f;
                            } else {
                                tree_19_score = 0.0392156862745098f;
                            }
                        } else {
                            if (features[10] <= -2.4127122163772583f) {
                                tree_19_score = 1.0f;
                            } else {
                                tree_19_score = 0.04296875f;
                            }
                        }
                    }
                } else {
                    tree_19_score = 1.0f;
                }
            } else {
                if (features[1] <= 0.132168211042881f) {
                    tree_19_score = 0.0f;
                } else {
                    if (features[2] <= -0.04246832896023989f) {
                        if (features[11] <= -0.015851765871047974f) {
                            tree_19_score = 1.0f;
                        } else {
                            tree_19_score = 0.0f;
                        }
                    } else {
                        if (features[11] <= -1.7690755724906921f) {
                            tree_19_score = 1.0f;
                        } else {
                            tree_19_score = 0.0f;
                        }
                    }
                }
            }
        } else {
            if (features[3] <= 0.1968986764550209f) {
                tree_19_score = 1.0f;
            } else {
                if (features[9] <= 3.7643654346466064f) {
                    if (features[11] <= -1.8323288559913635f) {
                        tree_19_score = 1.0f;
                    } else {
                        tree_19_score = 0.0f;
                    }
                } else {
                    tree_19_score = 1.0f;
                }
            }
        }
    total_score += tree_19_score;

    // Tree 20
    float tree_20_score = 0.0f;
        if (features[9] <= 3.2605810165405273f) {
            if (features[3] <= -2.3629446029663086f) {
                if (features[7] <= 0.8430981636047363f) {
                    if (features[11] <= 0.9794813096523285f) {
                        if (features[7] <= 0.5068013370037079f) {
                            if (features[6] <= -0.6517691314220428f) {
                                tree_20_score = 0.0f;
                            } else {
                                tree_20_score = 1.0f;
                            }
                        } else {
                            tree_20_score = 0.0f;
                        }
                    } else {
                        tree_20_score = 0.0f;
                    }
                } else {
                    tree_20_score = 1.0f;
                }
            } else {
                if (features[0] <= 2.8692221641540527f) {
                    if (features[10] <= 3.526843547821045f) {
                        if (features[7] <= 2.6211835145950317f) {
                            if (features[0] <= 0.6775429248809814f) {
                                tree_20_score = 0.0007710100231303007f;
                            } else {
                                tree_20_score = 0.026954177897574125f;
                            }
                        } else {
                            if (features[6] <= 3.1308066844940186f) {
                                tree_20_score = 0.13043478260869565f;
                            } else {
                                tree_20_score = 1.0f;
                            }
                        }
                    } else {
                        tree_20_score = 1.0f;
                    }
                } else {
                    if (features[11] <= 0.5366941690444946f) {
                        if (features[3] <= -1.9945863485336304f) {
                            if (features[1] <= 2.0626057982444763f) {
                                tree_20_score = 1.0f;
                            } else {
                                tree_20_score = 0.0f;
                            }
                        } else {
                            tree_20_score = 1.0f;
                        }
                    } else {
                        if (features[6] <= 1.2769663333892822f) {
                            tree_20_score = 1.0f;
                        } else {
                            tree_20_score = 0.0f;
                        }
                    }
                }
            }
        } else {
            if (features[9] <= 3.996321201324463f) {
                if (features[0] <= 1.3983873128890991f) {
                    tree_20_score = 0.0f;
                } else {
                    tree_20_score = 1.0f;
                }
            } else {
                tree_20_score = 1.0f;
            }
        }
    total_score += tree_20_score;

    // Tree 21
    float tree_21_score = 0.0f;
        if (features[1] <= 0.8854570984840393f) {
            if (features[3] <= -2.2841001749038696f) {
                if (features[7] <= -0.5818532407283783f) {
                    if (features[5] <= -0.7391137480735779f) {
                        tree_21_score = 1.0f;
                    } else {
                        tree_21_score = 0.0f;
                    }
                } else {
                    if (features[3] <= -2.9826403856277466f) {
                        tree_21_score = 1.0f;
                    } else {
                        if (features[3] <= -2.886755585670471f) {
                            tree_21_score = 0.0f;
                        } else {
                            tree_21_score = 1.0f;
                        }
                    }
                }
            } else {
                if (features[3] <= 1.8873828649520874f) {
                    if (features[7] <= 5.311134219169617f) {
                        if (features[10] <= 8.760403156280518f) {
                            if (features[10] <= 0.8120128512382507f) {
                                tree_21_score = 0.0043859649122807015f;
                            } else {
                                tree_21_score = 0.05217391304347826f;
                            }
                        } else {
                            tree_21_score = 1.0f;
                        }
                    } else {
                        tree_21_score = 1.0f;
                    }
                } else {
                    if (features[9] <= 3.6718677282333374f) {
                        if (features[11] <= -1.5502321124076843f) {
                            tree_21_score = 1.0f;
                        } else {
                            tree_21_score = 0.0f;
                        }
                    } else {
                        tree_21_score = 1.0f;
                    }
                }
            }
        } else {
            if (features[7] <= 1.708558440208435f) {
                if (features[11] <= -1.2966139316558838f) {
                    if (features[0] <= 3.3145676851272583f) {
                        if (features[3] <= 1.6715532541275024f) {
                            tree_21_score = 0.0f;
                        } else {
                            tree_21_score = 1.0f;
                        }
                    } else {
                        tree_21_score = 1.0f;
                    }
                } else {
                    if (features[0] <= 2.79851758480072f) {
                        if (features[1] <= 3.0636526346206665f) {
                            if (features[3] <= -2.1023181676864624f) {
                                tree_21_score = 0.2f;
                            } else {
                                tree_21_score = 0.0f;
                            }
                        } else {
                            if (features[5] <= -0.5395022630691528f) {
                                tree_21_score = 0.6f;
                            } else {
                                tree_21_score = 0.0f;
                            }
                        }
                    } else {
                        tree_21_score = 1.0f;
                    }
                }
            } else {
                if (features[8] <= 2.473878264427185f) {
                    if (features[4] <= 1.5630794763565063f) {
                        if (features[11] <= 0.10563351586461067f) {
                            tree_21_score = 1.0f;
                        } else {
                            tree_21_score = 0.0f;
                        }
                    } else {
                        tree_21_score = 0.0f;
                    }
                } else {
                    if (features[0] <= -0.0927261933684349f) {
                        if (features[8] <= 5.470617175102234f) {
                            tree_21_score = 0.0f;
                        } else {
                            tree_21_score = 1.0f;
                        }
                    } else {
                        tree_21_score = 1.0f;
                    }
                }
            }
        }
    total_score += tree_21_score;

    // Tree 22
    float tree_22_score = 0.0f;
        if (features[9] <= 3.318726062774658f) {
            if (features[3] <= -2.1327807903289795f) {
                if (features[10] <= 6.3477418422698975f) {
                    if (features[0] <= 3.84046733379364f) {
                        if (features[1] <= -0.26038849726319313f) {
                            tree_22_score = 1.0f;
                        } else {
                            tree_22_score = 0.0f;
                        }
                    } else {
                        tree_22_score = 1.0f;
                    }
                } else {
                    tree_22_score = 1.0f;
                }
            } else {
                if (features[4] <= 3.035856604576111f) {
                    if (features[1] <= 0.9763353765010834f) {
                        if (features[10] <= 4.652936220169067f) {
                            if (features[2] <= -2.0554758310317993f) {
                                tree_22_score = 0.75f;
                            } else {
                                tree_22_score = 0.010241404535479151f;
                            }
                        } else {
                            if (features[11] <= -1.8312214016914368f) {
                                tree_22_score = 1.0f;
                            } else {
                                tree_22_score = 0.0f;
                            }
                        }
                    } else {
                        if (features[9] <= 2.3433094024658203f) {
                            if (features[11] <= -1.2966139316558838f) {
                                tree_22_score = 0.5094339622641509f;
                            } else {
                                tree_22_score = 0.05976095617529881f;
                            }
                        } else {
                            if (features[11] <= -0.6155049502849579f) {
                                tree_22_score = 1.0f;
                            } else {
                                tree_22_score = 0.0f;
                            }
                        }
                    }
                } else {
                    if (features[0] <= 0.22199901938438416f) {
                        tree_22_score = 0.0f;
                    } else {
                        tree_22_score = 1.0f;
                    }
                }
            }
        } else {
            if (features[5] <= 3.997105360031128f) {
                if (features[3] <= -0.636979877948761f) {
                    tree_22_score = 1.0f;
                } else {
                    if (features[0] <= 1.0725319683551788f) {
                        tree_22_score = 0.0f;
                    } else {
                        tree_22_score = 1.0f;
                    }
                }
            } else {
                tree_22_score = 1.0f;
            }
        }
    total_score += tree_22_score;

    // Tree 23
    float tree_23_score = 0.0f;
        if (features[7] <= 2.268409490585327f) {
            if (features[0] <= 3.4795305728912354f) {
                if (features[3] <= -2.3715070486068726f) {
                    if (features[1] <= -0.18382518365979195f) {
                        if (features[9] <= -0.5912607312202454f) {
                            tree_23_score = 0.0f;
                        } else {
                            tree_23_score = 1.0f;
                        }
                    } else {
                        tree_23_score = 0.0f;
                    }
                } else {
                    if (features[10] <= 3.4539400339126587f) {
                        if (features[4] <= 3.2499619722366333f) {
                            if (features[5] <= 0.41080285608768463f) {
                                tree_23_score = 0.0008665511265164644f;
                            } else {
                                tree_23_score = 0.029914529914529916f;
                            }
                        } else {
                            tree_23_score = 1.0f;
                        }
                    } else {
                        if (features[5] <= 0.13540148735046387f) {
                            tree_23_score = 0.0f;
                        } else {
                            tree_23_score = 1.0f;
                        }
                    }
                }
            } else {
                tree_23_score = 1.0f;
            }
        } else {
            if (features[5] <= 3.3532897233963013f) {
                if (features[3] <= -3.4987685084342957f) {
                    tree_23_score = 1.0f;
                } else {
                    if (features[3] <= -0.18682093173265457f) {
                        tree_23_score = 0.0f;
                    } else {
                        if (features[11] <= -0.7441372275352478f) {
                            tree_23_score = 1.0f;
                        } else {
                            tree_23_score = 0.0f;
                        }
                    }
                }
            } else {
                if (features[9] <= 3.996321201324463f) {
                    if (features[8] <= 4.04364013671875f) {
                        tree_23_score = 1.0f;
                    } else {
                        tree_23_score = 0.0f;
                    }
                } else {
                    tree_23_score = 1.0f;
                }
            }
        }
    total_score += tree_23_score;

    // Tree 24
    float tree_24_score = 0.0f;
        if (features[5] <= 3.1783396005630493f) {
            if (features[1] <= 2.5411875247955322f) {
                if (features[0] <= 3.0216574668884277f) {
                    if (features[4] <= -1.6871748566627502f) {
                        if (features[6] <= 0.6194785833358765f) {
                            if (features[11] <= 0.48026490211486816f) {
                                tree_24_score = 0.0f;
                            } else {
                                tree_24_score = 0.2f;
                            }
                        } else {
                            tree_24_score = 1.0f;
                        }
                    } else {
                        if (features[10] <= 3.641140341758728f) {
                            if (features[2] <= -2.0554758310317993f) {
                                tree_24_score = 1.0f;
                            } else {
                                tree_24_score = 0.007402837754472548f;
                            }
                        } else {
                            if (features[1] <= 0.22143776342272758f) {
                                tree_24_score = 0.0f;
                            } else {
                                tree_24_score = 1.0f;
                            }
                        }
                    }
                } else {
                    tree_24_score = 1.0f;
                }
            } else {
                if (features[11] <= -1.5869125723838806f) {
                    tree_24_score = 1.0f;
                } else {
                    if (features[4] <= 3.049095630645752f) {
                        if (features[0] <= 3.8492270708084106f) {
                            tree_24_score = 0.0f;
                        } else {
                            tree_24_score = 1.0f;
                        }
                    } else {
                        if (features[7] <= 1.2911253720521927f) {
                            tree_24_score = 1.0f;
                        } else {
                            tree_24_score = 0.0f;
                        }
                    }
                }
            }
        } else {
            if (features[8] <= 3.193642258644104f) {
                tree_24_score = 0.0f;
            } else {
                tree_24_score = 1.0f;
            }
        }
    total_score += tree_24_score;

    // Tree 25
    float tree_25_score = 0.0f;
        if (features[9] <= 3.079315423965454f) {
            if (features[0] <= 2.8730103969573975f) {
                if (features[10] <= 3.4539400339126587f) {
                    if (features[11] <= -2.3724417686462402f) {
                        if (features[0] <= 0.755045410245657f) {
                            tree_25_score = 0.0f;
                        } else {
                            tree_25_score = 1.0f;
                        }
                    } else {
                        if (features[10] <= 0.6586642563343048f) {
                            if (features[7] <= 0.8529668152332306f) {
                                tree_25_score = 0.0f;
                            } else {
                                tree_25_score = 0.011857707509881422f;
                            }
                        } else {
                            if (features[2] <= -1.9128723740577698f) {
                                tree_25_score = 1.0f;
                            } else {
                                tree_25_score = 0.018469656992084433f;
                            }
                        }
                    }
                } else {
                    if (features[7] <= 0.9631990790367126f) {
                        tree_25_score = 0.0f;
                    } else {
                        tree_25_score = 1.0f;
                    }
                }
            } else {
                if (features[1] <= -0.5644227266311646f) {
                    tree_25_score = 0.0f;
                } else {
                    if (features[6] <= 2.5835214853286743f) {
                        tree_25_score = 1.0f;
                    } else {
                        if (features[7] <= 2.4602948427200317f) {
                            if (features[6] <= 2.7426544427871704f) {
                                tree_25_score = 0.0f;
                            } else {
                                tree_25_score = 1.0f;
                            }
                        } else {
                            tree_25_score = 0.0f;
                        }
                    }
                }
            }
        } else {
            if (features[8] <= 3.1749064922332764f) {
                tree_25_score = 0.0f;
            } else {
                if (features[9] <= 3.996321201324463f) {
                    if (features[0] <= 1.0597132742404938f) {
                        tree_25_score = 0.0f;
                    } else {
                        tree_25_score = 1.0f;
                    }
                } else {
                    tree_25_score = 1.0f;
                }
            }
        }
    total_score += tree_25_score;

    // Tree 26
    float tree_26_score = 0.0f;
        if (features[0] <= 2.2992312908172607f) {
            if (features[6] <= 3.8148739337921143f) {
                if (features[7] <= 3.7836374044418335f) {
                    if (features[4] <= -1.7288550734519958f) {
                        if (features[1] <= -1.0900747179985046f) {
                            tree_26_score = 0.0f;
                        } else {
                            if (features[2] <= -2.29419481754303f) {
                                tree_26_score = 1.0f;
                            } else {
                                tree_26_score = 0.5f;
                            }
                        }
                    } else {
                        if (features[3] <= -2.555559277534485f) {
                            if (features[11] <= -0.5145908743143082f) {
                                tree_26_score = 0.0f;
                            } else {
                                tree_26_score = 1.0f;
                            }
                        } else {
                            if (features[3] <= -1.7826634645462036f) {
                                tree_26_score = 0.2727272727272727f;
                            } else {
                                tree_26_score = 0.010346926354230066f;
                            }
                        }
                    }
                } else {
                    tree_26_score = 1.0f;
                }
            } else {
                if (features[8] <= 4.2465386390686035f) {
                    tree_26_score = 0.0f;
                } else {
                    tree_26_score = 1.0f;
                }
            }
        } else {
            if (features[7] <= 1.5345539450645447f) {
                if (features[0] <= 3.077815890312195f) {
                    if (features[5] <= 1.3171502947807312f) {
                        tree_26_score = 0.0f;
                    } else {
                        if (features[11] <= -1.762087881565094f) {
                            tree_26_score = 1.0f;
                        } else {
                            tree_26_score = 0.0f;
                        }
                    }
                } else {
                    if (features[9] <= 1.6547998785972595f) {
                        tree_26_score = 1.0f;
                    } else {
                        if (features[8] <= 2.665417432785034f) {
                            tree_26_score = 0.0f;
                        } else {
                            tree_26_score = 1.0f;
                        }
                    }
                }
            } else {
                if (features[11] <= 0.5636565685272217f) {
                    tree_26_score = 1.0f;
                } else {
                    if (features[11] <= 0.5767387449741364f) {
                        tree_26_score = 0.0f;
                    } else {
                        tree_26_score = 1.0f;
                    }
                }
            }
        }
    total_score += tree_26_score;

    // Tree 27
    float tree_27_score = 0.0f;
        if (features[1] <= 0.8854570984840393f) {
            if (features[0] <= 1.6006763577461243f) {
                if (features[9] <= 5.565526008605957f) {
                    if (features[2] <= -2.028870940208435f) {
                        if (features[1] <= -0.9950070977210999f) {
                            if (features[2] <= -2.5238672494888306f) {
                                tree_27_score = 1.0f;
                            } else {
                                tree_27_score = 0.0f;
                            }
                        } else {
                            tree_27_score = 1.0f;
                        }
                    } else {
                        if (features[3] <= -0.849554181098938f) {
                            if (features[1] <= 0.08308467455208302f) {
                                tree_27_score = 0.004629629629629629f;
                            } else {
                                tree_27_score = 0.20689655172413793f;
                            }
                        } else {
                            if (features[7] <= 2.6211835145950317f) {
                                tree_27_score = 0.0f;
                            } else {
                                tree_27_score = 0.037037037037037035f;
                            }
                        }
                    }
                } else {
                    tree_27_score = 1.0f;
                }
            } else {
                if (features[5] <= 1.9479431509971619f) {
                    if (features[0] <= 2.5096230506896973f) {
                        if (features[4] <= -1.4996800422668457f) {
                            tree_27_score = 1.0f;
                        } else {
                            tree_27_score = 0.0f;
                        }
                    } else {
                        if (features[7] <= -0.7138263881206512f) {
                            tree_27_score = 1.0f;
                        } else {
                            if (features[8] <= 1.2050253748893738f) {
                                tree_27_score = 0.0f;
                            } else {
                                tree_27_score = 1.0f;
                            }
                        }
                    }
                } else {
                    if (features[5] <= 3.3336888551712036f) {
                        if (features[7] <= 2.6854432821273804f) {
                            tree_27_score = 1.0f;
                        } else {
                            tree_27_score = 0.0f;
                        }
                    } else {
                        tree_27_score = 1.0f;
                    }
                }
            }
        } else {
            if (features[10] <= -1.8725054264068604f) {
                if (features[1] <= 1.2796779870986938f) {
                    if (features[1] <= 1.272229254245758f) {
                        if (features[8] <= 4.926132798194885f) {
                            if (features[10] <= -3.045572876930237f) {
                                tree_27_score = 1.0f;
                            } else {
                                tree_27_score = 0.0f;
                            }
                        } else {
                            tree_27_score = 1.0f;
                        }
                    } else {
                        tree_27_score = 0.0f;
                    }
                } else {
                    if (features[10] <= -2.0522072315216064f) {
                        if (features[5] <= 3.0020071268081665f) {
                            if (features[10] <= -3.166251301765442f) {
                                tree_27_score = 1.0f;
                            } else {
                                tree_27_score = 0.8f;
                            }
                        } else {
                            tree_27_score = 1.0f;
                        }
                    } else {
                        if (features[10] <= -2.023908495903015f) {
                            tree_27_score = 0.0f;
                        } else {
                            tree_27_score = 1.0f;
                        }
                    }
                }
            } else {
                if (features[10] <= 3.4489299058914185f) {
                    if (features[0] <= 2.880187511444092f) {
                        if (features[3] <= 3.8136483430862427f) {
                            if (features[9] <= 2.6102476119995117f) {
                                tree_27_score = 0.0035714285714285713f;
                            } else {
                                tree_27_score = 1.0f;
                            }
                        } else {
                            tree_27_score = 1.0f;
                        }
                    } else {
                        if (features[10] <= 2.533681035041809f) {
                            tree_27_score = 1.0f;
                        } else {
                            if (features[5] <= 6.751301825046539f) {
                                tree_27_score = 0.0f;
                            } else {
                                tree_27_score = 1.0f;
                            }
                        }
                    }
                } else {
                    tree_27_score = 1.0f;
                }
            }
        }
    total_score += tree_27_score;

    // Tree 28
    float tree_28_score = 0.0f;
        if (features[0] <= 2.386655569076538f) {
            if (features[9] <= 4.04762601852417f) {
                if (features[3] <= -2.3711849451065063f) {
                    if (features[1] <= -0.2557939328253269f) {
                        tree_28_score = 1.0f;
                    } else {
                        tree_28_score = 0.0f;
                    }
                } else {
                    if (features[10] <= 3.651718258857727f) {
                        if (features[4] <= -1.738483190536499f) {
                            if (features[3] <= -1.4602688550949097f) {
                                tree_28_score = 1.0f;
                            } else {
                                tree_28_score = 0.0f;
                            }
                        } else {
                            if (features[4] <= 3.2525634765625f) {
                                tree_28_score = 0.009259259259259259f;
                            } else {
                                tree_28_score = 1.0f;
                            }
                        }
                    } else {
                        if (features[5] <= 0.9078077524900436f) {
                            tree_28_score = 0.0f;
                        } else {
                            tree_28_score = 1.0f;
                        }
                    }
                }
            } else {
                tree_28_score = 1.0f;
            }
        } else {
            if (features[7] <= 1.5345539450645447f) {
                if (features[3] <= -0.7374567091464996f) {
                    if (features[6] <= -0.46139781177043915f) {
                        tree_28_score = 0.0f;
                    } else {
                        if (features[1] <= 0.8615936040878296f) {
                            if (features[10] <= -2.2875107526779175f) {
                                tree_28_score = 1.0f;
                            } else {
                                tree_28_score = 0.1875f;
                            }
                        } else {
                            if (features[0] <= 3.6475738286972046f) {
                                tree_28_score = 0.0f;
                            } else {
                                tree_28_score = 1.0f;
                            }
                        }
                    }
                } else {
                    tree_28_score = 1.0f;
                }
            } else {
                tree_28_score = 1.0f;
            }
        }
    total_score += tree_28_score;

    // Tree 29
    float tree_29_score = 0.0f;
        if (features[7] <= 2.269965648651123f) {
            if (features[11] <= -1.8583617210388184f) {
                if (features[6] <= 0.3644398972392082f) {
                    if (features[11] <= -1.877633273601532f) {
                        if (features[7] <= -1.2319844961166382f) {
                            tree_29_score = 1.0f;
                        } else {
                            tree_29_score = 0.0f;
                        }
                    } else {
                        tree_29_score = 1.0f;
                    }
                } else {
                    if (features[4] <= -0.30498287454247475f) {
                        if (features[7] <= 1.8847108483314514f) {
                            tree_29_score = 1.0f;
                        } else {
                            tree_29_score = 0.0f;
                        }
                    } else {
                        tree_29_score = 1.0f;
                    }
                }
            } else {
                if (features[10] <= -2.9541901350021362f) {
                    tree_29_score = 1.0f;
                } else {
                    if (features[2] <= -2.149408459663391f) {
                        if (features[10] <= -0.8695377260446548f) {
                            tree_29_score = 0.0f;
                        } else {
                            tree_29_score = 1.0f;
                        }
                    } else {
                        if (features[8] <= 2.9917036294937134f) {
                            if (features[1] <= 4.17370343208313f) {
                                tree_29_score = 0.016443361753958587f;
                            } else {
                                tree_29_score = 1.0f;
                            }
                        } else {
                            if (features[7] <= -0.651102701202035f) {
                                tree_29_score = 0.0f;
                            } else {
                                tree_29_score = 1.0f;
                            }
                        }
                    }
                }
            }
        } else {
            if (features[5] <= 3.633243203163147f) {
                if (features[7] <= 2.786025047302246f) {
                    if (features[0] <= 1.2926724553108215f) {
                        if (features[11] <= -1.833194077014923f) {
                            tree_29_score = 1.0f;
                        } else {
                            tree_29_score = 0.0f;
                        }
                    } else {
                        if (features[4] <= 1.999068260192871f) {
                            tree_29_score = 1.0f;
                        } else {
                            tree_29_score = 0.0f;
                        }
                    }
                } else {
                    if (features[0] <= 1.7649414539337158f) {
                        tree_29_score = 0.0f;
                    } else {
                        if (features[9] <= 2.7881240844726562f) {
                            tree_29_score = 0.0f;
                        } else {
                            tree_29_score = 1.0f;
                        }
                    }
                }
            } else {
                if (features[8] <= 4.1572136878967285f) {
                    if (features[6] <= 3.651055693626404f) {
                        tree_29_score = 1.0f;
                    } else {
                        tree_29_score = 0.0f;
                    }
                } else {
                    tree_29_score = 1.0f;
                }
            }
        }
    total_score += tree_29_score;

    return total_score / 30.0f;
}

#endif
