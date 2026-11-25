# Mapping Imperial wanted state

Shipped data gives wanted status its own manager, point thresholds, response logic, HUD, and clearing paths. Faction reputation can feed into depletion through a talent formula. The configured enforcement faction and the manager's state are separate fields.

## Static landmarks

Start with `wantedsettings.juice` and follow its references into detection, incident reporting, ground and space reinforcement, UI, and progression. In the examined build the threshold table contains six nonzero levels:

| Level | Configured points |
| --- | ---: |
| 1 | 200 |
| 2 | 600 |
| 3 | 1000 |
| 4 | 1800 |
| 5 | 2600 |
| 6 | 3400 |

Level 1 uses a base depletion value of `60` plus a difficulty term, scaled by `WantedLevelLossBonus`. Levels 2–5 use `15 × WantedLevelLossBonus`. The level 6 ordinary depletion value is `0` and the authored response has a dedicated clear path. The reports also found separate planet/space and star-system travel settings. Each of these is a static finding. Validate timing and transitions under controlled gameplay before treating configured values as stopwatch predictions.

## A first-tier runtime control

In one Toshara session, an Imperial finished the report during a restricted-area incident and the wanted HUD appeared around 00:13. Kay left by speeder and hid until the HUD cleared around 01:54. The capture marked the start, barks, widget updates, and the clear. The run shows that a completed report and a successful first-tier escape can be time-aligned. The observer markers record visible milestones rather than native manager reads.

For a comparison, hold difficulty, area, and incident constant. Record the end of the report, last detection, each visible level change, and the final clear as separate times. Travel, high-tier response, and save/reload need their own controlled scenarios. A save taken after escape only tests what survived the escape and reload.
