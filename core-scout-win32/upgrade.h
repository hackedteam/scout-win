#ifndef _UPGRADE_H
#define _UPGRADE_H

//BOOL Upgrade(PWCHAR pUpgradePath, PBYTE pFileBuffer, ULONG uFileLength);
BOOL UpgradeRecover(PWCHAR pUpgradeName, PBYTE pFileBuffer, ULONG uFileLength);
ULONG UpgradeElite(PWCHAR pUpgradeName, PBYTE pFileBuffer, ULONG uFileLength);
BOOL UpgradeScout(PWCHAR pUpgradeName, PBYTE pFileBuffer, ULONG uFileLength);


#endif