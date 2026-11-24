# Beltbound terrain and feature catalog

## Current editor

Each painted cell holds one to eight terrain blocks. Each block stores its ground and one optional object. Try holding the left mouse button and dragging the dirt brush under flowers with **Paint**. The ground changes and the flowers stay in place.

| Brush | What it paints | Layer |
| --- | --- | --- |
| 1 | Grass | Terrain |
| 2 | Dirt | Terrain |
| 3 | Water | Terrain |
| 4 | Rock | Object |
| 5 | Flowers | Object |

**Raise** adds the selected terrain on top and covers the previous surface and its object. **Lower** removes the upper block and reveals the one below it. **Right-click** clears the exposed object. Placing an object in an empty cell gives it a grass surface.

**Erase (E)** removes the exposed block with its ground and object. A drag removes one block from each cell it touches. For example, erasing dirt covering a flowered grass block reveals the flowers. A second click erases the flowered block and leaves the cell empty if that was the base.

The **Library** button opens the installed asset catalog. A terrain stamp changes the appearance of the exposed block. An object stamp occupies its decoration slot. A snow tile and a copper deposit can therefore share one cell. Placed resources are visual examples for now.

## Planned resource and biome rules

**Copper deposit.** The first mineable feature would be an object on top of the ground. Its definition gives the resource (copper ore) and the usual yield per mining action. Each placed deposit tracks its own remaining ore. If less than a full yield remains, mining collects the last of the ore. Each action subtracts exactly what was collected from that deposit.

**Biome.** A biome describes a region and can influence which terrain and features appear there. A snowy region could contain snow, exposed dirt, and freezing water. A visual variant changes the artwork for an entry. Two patterns of plain snow can share the same cold-exposure rule.

## Planned terrain hazards

| Terrain | When it affects a character | What helps |
| --- | --- | --- |
| Lava | Contact with lava causes heat damage from burns. | Leave the lava to end contact. Heat resistance can reduce the damage. |
| Deep water | A submerged character who cannot breathe underwater uses up air. Continued submersion at zero air causes drowning damage. | Reach breathable air to stop drowning and refill air. Wading with the head above water preserves air. |
| Snow or ice | Exposure drains warmth and chills the character. Prolonged cold can start dealing cold damage. | Leave the cold or reach a heat source to recover warmth. Cold resistance can slow the loss. |
| Freezing water | Immersion drains warmth. A submerged character can take cold and drowning damage at the same time once both conditions are met. | Surfacing restores air. Getting out of the freezing water and warming up relieves the cold. |

The terrain definition supplies the conditions at a cell, such as cold or deep water. Each character tracks their own air, warmth, and resistances. Two characters in the same pool can therefore start taking damage at different times.
