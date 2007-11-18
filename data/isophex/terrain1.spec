
[spec]

; Format and options of this spec file:
options = "+spec3"

[info]

artists = "
    Tim F. Smith <yoohootim@hotmail.com>
    Andreas Røsdal <andrearo@pvv.ntnu.no> (hex mode)
    Daniel Speyer <dspeyer@users.sf.net> 
"

[file]
gfx = "isophex/terrain1"

[grid_main]

x_top_left = 1
y_top_left = 1
dx = 64
dy = 32
pixel_border = 1

tiles = { "row", "column","tag"

; terrain
  0,    0,  "t.desert1"
;  0,    1,  "t.desert2"

  1,    0,  "t.plains1"
;  1,    1,  "t.plains2"

  2,    0,  "t.grassland1"
;  2,    1, "t.grassland2"

  3,    0, "t.forest1"
;  3,    1, "t.forest2"

  4,    0, "t.hills1"
;  4,    1, "t.hills2"

  5,    0, "t.mountains1"
;  5,    1, "t.mountains2"

  6,    0, "t.tundra1"

  7,    0, "t.arctic1"

  8,    0, "t.swamp1"

  9,    0, "t.jungle1"

  10,   0, "t.ocean1"

; Terrain special resources:

 0,   2, "ts.oasis"
 0,   3, "ts.oil"

 1,   2, "ts.buffalo"
 1,   3, "ts.wheat"

 7,   7, "ts.grassland_resources", "ts.river_resources"

 3,   2, "ts.pheasant"
 3,   3, "ts.silk"

 4,   2, "ts.coal"
 4,   3, "ts.wine"

 5,   2, "ts.gold"
 5,   3, "ts.iron"

 6,   2, "ts.tundra_game"
 6,   3, "ts.furs"

 7,   2, "ts.arctic_ivory"
 7,   3, "ts.arctic_oil"

 8,   2, "ts.peat"
 8,   3, "ts.spice"

 9,   2, "ts.gems"
 9,   3, "ts.fruit"
 9,   5, "ts.seals"
 9,   6, "ts.forest_game"

 10,  2, "ts.fish"
 10,  3, "ts.whales"
 10,  5, "ts.horses"

;roads
 11, 1, "r.road_n"
 11, 2, "r.road_ne"
 11, 3, "r.road_e"
 11, 4, "r.road_se"
 11, 5, "r.road_s"
 11, 6, "r.road_sw"
 11, 7, "r.road_w"
 11, 8, "r.road_nw"

;rails
 12, 1, "r.rail_n"
 12, 2, "r.rail_ne"
 12, 3, "r.rail_e"
 12, 4, "r.rail_se"
 12, 5, "r.rail_s"
 12, 6, "r.rail_sw"
 12, 7, "r.rail_w"
 12, 8, "r.rail_nw"

;add-ons
 2, 7, "tx.oil_mine" 
 4, 7, "tx.farmland"
 3, 7, "tx.irrigation"
 5, 7, "tx.mine"
 6, 7, "tx.pollution"
 8, 7, "tx.village"
 9, 7, "tx.fallout"
}


[grid_extra]

x_top_left = 1
y_top_left = 447
dx = 64
dy = 32
pixel_border = 1

tiles = { "row", "column","tag"
  0, 0, "t.dither_tile"
  0, 0, "tx.darkness"
  0, 1, "tx.fog"
  0, 2, "t.black_tile"
  0, 3, "t.coast_color"

  0, 4, "user.attention"
}


[grid_isolated]

x_top_left = 380
y_top_left = 1
dx = 64
dy = 32
pixel_border = 1

tiles = { "row", "column","tag"
  0, 0, "r.road_isolated"
  1, 0, "r.rail_isolated"
}
