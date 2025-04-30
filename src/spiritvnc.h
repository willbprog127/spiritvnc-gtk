/*
 * spiritvnc.h - 2022-2025 Will Brokenbourgh
 * part of the SpiritVNC - Gtk project
 *
*/

/*
 * (C) Will Brokenbourgh
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SPIRITVNC_C_H
#define SPIRITVNC_C_H

#include <gtk/gtk.h>
#include <gtk-vnc-2.0/gtk-vnc.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#include <stdbool.h>

#include "xpms.h"

// forward declarations
typedef struct Connection Connection;
typedef struct ToolsMenuItems ToolsMenuItems;

// structs
typedef struct Application
{
  // Application
  GtkApplication * gApp;

  // important widgets
  GtkWidget * mainWin;
  GtkWidget * parent;
  GtkWidget * menuBar;
  GtkWidget * serverListScroller;
  GtkWidget * serverList;
  GtkWidget * displayStackScroller;
  GtkWidget * displayStack;
  GtkWidget * quickNoteLabel;
  GtkWidget * quickNoteLastConnected;
  GtkWidget * quickNoteLastError;
  GtkWidget * quickNoteView;

  // text buffers for quicknote stuffz
  GtkTextBuffer * quickNoteLastErrorBuffer;
  GtkTextBuffer * quickNoteBuffer;

  // objects
  GList * connections;
  ToolsMenuItems * toolsItems;
  Connection * selectedConnection;
  GString * f12Storage;

  // app properties
  int serverListWidth;
  bool showTooltips;
  bool logToFile;
  bool maximized;
  bool debugMode;
  unsigned int vncConnectWaitTime;
  unsigned int scanTimeout;
  bool addNewConnection;

  //# flags, states and stuff
  //mouse_click_type = None
  //startup = True
  //show_intro = False
  //is_fullscreen = False
  //is_resizing = False
  //is_writing_config = False
  //listen_mode = False

  // important paths that probably shouldn't be hard-coded
  GString * appConfigDir;
  GString * appConfigFile;
  GString * appLogFile;

  //# clipboard goodies
  //clip = Gtk.Clipboard()
  //server_clip = False
  //client_clip = False

  // ssh stuff
  GString * sshCommand;
  unsigned int sshConnectWaitTime;
  unsigned int sshShowWaitTime;

} App;

App * app;

typedef struct Connection
{
  GString * name;
  GString * group;
  GString * address;
  unsigned int type;
  GString * vncPort;
  GString * vncPass;
  GString * vncLoginUser;
  GString * vncLoginPass;
  //bool vncError;
  bool scale;
  bool lossyEncoding;
  bool showRemoteCursor;
  unsigned int quality;
  GString * sshUser;
  GString * sshPass;
  GString * sshPort;
  //GString * sshPubKeyfile;
  GString * sshPrivKeyfile;
  GString * f12Macro;
  GString * quickNote;
  bool customCmd1Enabled;
  GString * customCmd1Label;
  GString * customCmd1;
  bool customCmd2Enabled;
  GString * customCmd2Label;
  GString * customCmd2;
  bool customCmd3Enabled;
  GString * customCmd3Label;
  GString * customCmd3;
  // 'private' vars
  unsigned int state;
  GtkWidget * vncObj;
  unsigned int disconnectType;
  GString * lastErrorMessage;
  GString * lastConnectTime;
  unsigned int sshLocalPort;
  FILE * sshCmdStream;
  GThread * sshThread;
  GThread * sshMonitorThread;
  GThread * sshCloseThread;
  bool sshReady;
  FILE * sshStream;
  GString * clipboard;
  GtkWidget * settingsWin;
} Connection;

typedef struct
{
  Connection * con;
  GtkWidget * settingsWin;
  GtkWidget * connectionName;
  GtkWidget * connectionGroup;
  GtkWidget * remoteAddress;
  GtkWidget * f12Macro;
  GtkWidget * vncChoice;
  GtkWidget * svncChoice;
  GtkWidget * vncPort;
  GtkWidget * vncPassword;
  GtkWidget * vncLoginUsername;
  GtkWidget * vncLoginPassword;
  GtkWidget * vncQuality;
  GtkWidget * vncLossyEncoding;
  GtkWidget * vncScaling;
  GtkWidget * sshUsername;
  GtkWidget * sshPort;
  GtkWidget * sshPublicKey;
  GtkWidget * sshPrivateKey;
  GtkWidget * cmd1Enabled;
  GtkWidget * cmd1Label;
  GtkWidget * cmd1;
  GtkWidget * cmd2Enabled;
  GtkWidget * cmd2Label;
  GtkWidget * cmd2;
  GtkWidget * cmd3Enabled;
  GtkWidget * cmd3Label;
  GtkWidget * cmd3;
} ConnectionSettings;

typedef struct AppOptions
{
  GtkWidget * optionsWin;
  GtkWidget * showTooltips;
  GtkWidget * logToFile;
  GtkWidget * scanTimeout;
  GtkWidget * vncWaitTime;
  GtkWidget * sshWaitTime;
  GtkWidget * sshCommand;
} AppOptions;

typedef struct ToolsMenuItems
{
  GtkWidget * requestUpdate;
  GtkWidget * send;
  GtkWidget * sendEnteredKeys;
  GtkWidget * sendCAD;
  GtkWidget * sendCSE;
  GtkWidget * addNew;
  GtkWidget * fullscreen;
  GtkWidget * listenMode;
  GtkWidget * screenshot;
  GtkWidget * appOptions;
  GtkWidget * quit;
} ToolsMenuItems;

typedef struct SendKeysObj
{
  GtkWidget * win;
  GtkWidget * textView;
  Connection * con;
} SendKeysObj;

enum ConnectionState
{
  SV_STATE_DISCONNECTED = 0,
  SV_STATE_WAITING,
  SV_STATE_CONNECTED,
  SV_STATE_TIMEOUT,
  SV_STATE_ERROR
};

enum ConnectionType
{
  SV_TYPE_VNC = 0,
  SV_TYPE_VNC_OVER_SSH
};

enum ConnectionQuality
{
  SV_QUAL_LOW = 0,
  SV_QUAL_MEDIUM,
  SV_QUAL_FULL,
  SV_QUAL_DEFAULT
};

enum ConnectionDisconnectType
{
  SV_DISC_NONE = 0,
  SV_DISC_MANUAL,
  SV_DISC_TIMEOUT,
  SV_DISC_VNC_ERROR,
  SV_DISC_SSH_ERROR
};


/* functions */
void svDoQuit ();
void svCreateGUI (GtkApplication *);
void svConfigRead ();
void svConfigWrite ();
bool svConfigCreateNew (bool);
//Connection * svGetSelectedConnection ();
void svConnectionEnd (Connection *);
void svConnectionRightClick (Connection *);
void svConnectionCreate (Connection *);
Connection * svConnectionFromName (const char *);
void svConnectionSwitch (Connection *);
gpointer svCreateSSHConnection (gpointer);
gpointer svSSHMonitor (gpointer);
int svGetSelectedConnectionListItemIndex ();
Connection * svGetSelectedConnectionListConnection ();
void svHandleAddNewConnectionMenuItem ();
gboolean svHandleConnectionListClicks (GtkWidget *, GdkEvent *, void *);
void svHandleConnectionSSHPrivKey (GtkButton *, gpointer);
void svHandleDeleteMenuItem (GtkMenuItem *, gpointer);
void svHandleRequestUpdateMenuItem (GtkMenuItem *, gpointer);
void svHandleSendCADMenuItem (GtkMenuItem *, gpointer);
void svHandleSendCSEMenuItem (GtkMenuItem *, gpointer);
void svHandleSendEnteredKeystrokesMenuItem (GtkMenuItem *, gpointer);
void svHandleSendEnteredKeystrokesSend (GtkButton *, gpointer);
void svHandleScreenshotMenuItem ();
void svInitAppVars ();
void svInitConnObject (Connection *);
void svInsertHostListRow (char *, int);
void svLog (char *, bool);
void svConnectionOpen (Connection *);
void svToLower (char *);
void svSavePreviousQuickNoteText (Connection *);
void svServerError (VncConnection *, const char *, void *);
void svSetIconFromConnectionName (const char *, unsigned int);
void svSetToolsMenuItems(bool);
void svSetTooltip(GtkWidget *, const char *);
void svSetMenuItemTooltips ();
void svShowAppOptionsWindow ();
gpointer svSSHConnectionCloser (gpointer);
void svStartConnection (Connection *);
bool svStringToBool (char *);

#endif
