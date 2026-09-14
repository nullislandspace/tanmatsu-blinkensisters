// BlinkenSisters - Tanmatsu port
//
// In-game downloads. The app ships without game data: the base data and every
// addon are GitHub release assets listed in addons/index.json (published by
// tools/publish-addons.py), fetched over WiFi and unpacked on the device.

#ifndef ADDONSTORE_H
#define ADDONSTORE_H

// Make sure the base data is unpacked, downloading it if need be. Call after
// configInit() and before anything loads menu artwork. Returns false only if
// the player declined, in which case there is nothing the game can show.
bool addonStoreEnsureBaseData();

// Offer to download Lost Pixels, the main game, if it is not installed.
void addonStoreOfferLostPixels();

// The "Addons" menu: install, update and remove addons.
void addonStoreMenu();

#endif // ADDONSTORE_H
