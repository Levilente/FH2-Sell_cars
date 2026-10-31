# Sell Car for Forza Horizon 2

Adds a **SELL CAR** entry to the garage in Forza Horizon 2 (Xbox One build on PC, via the Azimuth launcher), built on the **FH2 Mod Platform**.

In **Garage > My Cars**, the action menu of a car (Get in car, Remove car, View car, ...) gets a new line right after **Remove car**: **SELL CAR**. Choosing it opens a small menu whose title shows what you would get (for example *SELL THIS CAR FOR 123,456 CR?*), with two entries you can select with the controller or click with the mouse: **SELL CAR** and **BACK**.

**Remove car is not changed**: it still removes the car for nothing. Selling is optional, so nobody has to play with a different economy.

## What you get for a car

```
sale price = base price of the model x BasePercent / 100
           + what you paid for the car's upgrades x UpgradePercent / 100
```

Defaults: **50%** of the model's price and **30%** of what was paid for upgrades. Both are settings (see below). The model price comes from the game's own car data and the upgrade total from the game's record of what you really paid for each part (the same data the game keeps per car).

After the sale the car is removed from the garage by the game's own removal code, exactly like Remove car, and the credits are added to your balance.

## Requirements

| What | Version |
|---|---|
| Forza Horizon 2 on PC with the **Azimuth** launcher | game build 1.0.0.64 (the one the platform targets) |
| **[FH2 Mod Platform](https://github.com/Trinculo54/fh2-mod-platform)** | **2.8.0 or newer** (tested with 2.8.0, API 9). The mod only uses API 3 functions (platform 2.2.0: hooks and code waiting), but nothing older than 2.8.0 has been tested. |
| `forza_durango_release_final.exe` of that game build | The mod checks the bytes of every function it hooks. If they do not match, the mod switches itself off, says so in the log and changes nothing. |

The platform is the `DWrite.dll` (and `d3dcompiler_47.dll`) next to `azimuth.exe`; if you use any other platform-based mod you already have it. If not, get it from the platform's [releases page](https://github.com/Trinculo54/fh2-mod-platform/releases) and read its README for which file your Azimuth version loads.

## Installation

1. Close the game.
2. Download `SellCar.dll` from the [latest release](../../releases/latest).
3. Copy it to `<your Azimuth folder>\fh2-mods\` (the folder next to `azimuth.exe`).
4. Start the game, open **Garage > My Cars**, pick a car and look for **SELL CAR** after **Remove car**.

To check that it loaded, open `fh2-mods\platform.log` and look for `[Sell Car]` lines:

```
SELL CAR active: Garage > My Cars > pick a car > the action menu now has SELL CAR after Remove car
SELL CAR entry added to the car action menu          (every time the menu opens, the first few times)
sell: car in garage slot 30
sell: price 123456 CR
sold a car: credits 500000 -> 623456 (+123456)
```

If a game function is not what the mod expects (another game build), it logs `... Sell car stays off (nothing was changed)` and does nothing.

### Uninstalling

Delete `SellCar.dll` (and `SellCar.ini`) from `fh2-mods`. The mod stores nothing in your save beyond what the game itself writes when a car is removed and credits change, so cars you already sold stay sold.

## Settings

`fh2-mods\SellCar.ini` is created on first start next to the DLL:

| Key | Default | Meaning |
|---|---|---|
| `[Sell] BasePercent` | `50` | Percent of the model's base price you get (0 to 100). |
| `[Sell] UpgradePercent` | `30` | Percent of what you paid for the car's upgrades that you get (0 to 100). |
| `[Sell] CreditReason` | `7` | The reason code the game writes into its credit history for the payment (7 = *refund car*). If your credit history looks odd, try `3`. |

Restart the game after changing the file.

## Known limitations

- A sale cannot be undone. The confirmation menu is there for that reason; use **BACK** if unsure.
- The price is a fixed percentage of the *list* price, and any discount or bonus the game itself applies to car prices (perks, free or gifted cars) is not taken into account. A car you got for free still sells for the percentage of its list price.
- Only the payment and the removal are done by this mod. Statistics such as the "times sold" counter of the car are not touched.
- Tested on the Xbox One build 1.0.0.64 with platform 2.8.0 only.

## How it works

No game file is touched; everything happens in memory through hooks of the mod platform.

1. The popup that lists a car's actions is built by one game function. While it runs, the list's `AddItem` is watched, and right after the *Remove car* entry a second entry (tag 5, *SELL CAR*) is added with the game's own item-creation functions. Its text comes from the game's string lookup, which the mod hooks for its own text ids.
2. Choosing that entry reads the price with one SQL query against the game's database (base price of the model plus the sum of `PricePaid` of the car's parts, with your percentages) and then asks the game to build the same kind of popup once more, but with only two entries, *SELL CAR* and *BACK*, and the price in the title.
3. *SELL CAR* adds the credits with the game's own credit functions (read the balance, write the new one with a reason code) and then calls the game's car removal code with the same data its own confirmation would have passed. *BACK* just closes the menu.

## Building from source

You need [Zig 0.15.2](https://ziglang.org/download/) (used as a C cross compiler; nothing else).

```bat
BUILD_WINDOWS.cmd
```

(or drop `zig.exe` onto the file) produces `dist\SellCar.dll`. The mod is written in freestanding C (`-ffreestanding`, no C runtime, no extra DLL imports beyond `kernel32` / `user32`).

Host tests (gcc, any OS; they cover the text, number and SQL building; the clang syntax check of the game-facing code is run when clang is installed):

```sh
sh tests/run_tests.sh
```

## Credits and thanks

- **[Trinculo54](https://github.com/Trinculo54)** for the **[FH2 Mod Platform](https://github.com/Trinculo54/fh2-mod-platform)** (MIT), which everything here runs on. `include/fh2mod.h` is the platform's public header under its original MIT licence ([LICENSE-fh2mod-MIT.txt](LICENSE-fh2mod-MIT.txt)).
- **[RokkuDayo](https://github.com/RokkuDayo)** for **[Forza Horizon 2 Extra Options](https://github.com/RokkuDayo/forzahorizon2_extraoptions)**, which made modding the game's media possible in the first place.
- **Levilente**: this mod.

See [CREDITS.md](CREDITS.md).

## License

**MIT**, see [LICENSE](LICENSE). `include/fh2mod.h` stays MIT (Trinculo54).

This is an unofficial fan modification. Forza Horizon 2 is a trademark of Microsoft / Playground Games; the mod contains no game assets and does not modify the game's files.
