/*
 * spiritvnc.h - 2022-2026 Will Brokenbourgh
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

#define SV_APP_VERSION "0.0.3"

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

  // toolbar controls
  GtkWidget * toolbuttonListen;
  GtkWidget * listenImage;
  GtkWidget * toolbuttonScan;
  GtkWidget * scanImage;
  GtkWidget * addConnectionToolbutton;
  GtkWidget * addConnectionImage;

  // text buffers for quicknote stuffz
  GtkTextBuffer * quickNoteLastErrorBuffer;
  GtkTextBuffer * quickNoteBuffer;

  GtkWidget * connectionActionsWindow;

  // objects
  //ToolsMenuItems * toolsItems;
  GHashTable * toolsItems;
  Connection * selectedConnection;
  GString * f12Storage;

  // app properties
  gint serverListWidth;
  gint serverListWidthLast;
  gboolean showTooltips;
  gboolean logToFile;
  gboolean maximized;
  gboolean fullscreen;
  gboolean scanMode;
  guint scanTimeout;
  gboolean debugMode;
  guint vncConnectWaitTime;
  gboolean addNewConnection;

  //# flags, states and stuff
  gboolean listenMode;
  guint listenPort;
  GSocket * listenSocket;
  GSource * listenSource;
  guint scanTimerSource;

  // important paths
  GString * appConfigDir;
  GString * appConfigFile;
  GString * appLogFile;

  // ssh stuff
  GString * sshCommand;
  guint sshConnectWaitTime;
  guint sshShowWaitTime;

} App;

App * app;

typedef struct Connection
{
  GString * name;
  GString * group;
  GString * address;
  guint type;
  GString * vncPort;
  GString * vncPass;
  GString * vncLoginUser;
  GString * vncLoginPass;
  //gboolean vncError;
  gboolean scale;
  gboolean lossyEncoding;
  gboolean showRemoteCursor;
  guint quality;
  GString * sshUser;
  GString * sshPass;
  GString * sshPort;
  GString * sshPrivKeyfile;
  GString * f12Macro;
  GString * quickNote;
  gboolean customCmd1Enabled;
  GString * customCmd1Label;
  GString * customCmd1;
  gboolean customCmd2Enabled;
  GString * customCmd2Label;
  GString * customCmd2;
  gboolean customCmd3Enabled;
  GString * customCmd3Label;
  GString * customCmd3;
  // 'private' vars
  guint state;
  GtkWidget * vncObj;
  guint disconnectType;
  GString * lastErrorMessage;
  GString * lastConnectTime;
  guint sshLocalPort;
  GThread * sshThread;
  GThread * sshMonitorThread;
  GThread * sshCloseThread;
  gboolean sshContinue;
  GPid sshPid;
  gint sshStdIn;
  GString * clipboard;
  GtkWidget * settingsWin;
  gint listenFd;
  gboolean viewOnly;
} Connection;

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
  SV_TYPE_VNC_OVER_SSH,
  SV_TYPE_VNC_REVERSE
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

enum SendKeysObjectType
{
  SV_SENDKEYS_TYPE_ENTRY,
  SV_SENDKEYS_TYPE_OTHER
};

/* functions */
void svDoQuit ();
void svCancelScanMode (gboolean);
void svCreateGUI (GtkApplication *);
void svConfigRead ();
void svConfigWrite ();
gboolean svConfigCreateNew (gboolean);
Connection * svGetSelectedConnection ();
void svConnectionEnd (Connection *);
void svConnectionRightClick (Connection *);
void svConnectionCreate (Connection *);
Connection * svConnectionFromName (const char *);
void svConnectionSwitch (Connection *);
gpointer svCreateSSHConnection (gpointer);
gpointer svSSHMonitor (gpointer);
GdkFilterReturn svEventFilter(GdkXEvent *, GdkEvent *, gpointer);
gboolean svFocusOnce (gpointer);
void svFreeConnObject(Connection *);
gboolean svThereAreConnectedConnections ();
Connection * svGetSelectedConnectionListConnection ();
void svHandleAddNewConnectionMenuItem (GtkMenuItem *, gpointer);
void svHandleConnectionSettingsButtons (GtkButton *, gpointer);
gboolean svHandleConnectionListClicks (GtkWidget *, GdkEvent *, void *);
void svHandleConnectionSSHPrivKey (GtkButton *, gpointer);
void svHandleDeleteMenuItem (GtkMenuItem *, gpointer);
void svHandleFullscreenMenuItem (GtkMenuItem *, gpointer);
void svHandleRequestUpdateMenuItem (GtkMenuItem *, gpointer);
void svHandleScanModeMenuItem (GtkMenuItem *, gpointer);
void svHandleSendCADMenuItem (GtkMenuItem *, gpointer);
void svHandleSendCSEMenuItem (GtkMenuItem *, gpointer);
void svHandleSendEnteredKeystrokesMenuItem (GtkMenuItem *, gpointer);
void svHandleSendEnteredKeystrokesButtons (GtkButton *, gpointer);
void svHandleScreenshotMenuItem (GtkMenuItem *, gpointer);
void svInitAppVars ();
void svInitConnObject (Connection *);
void svInsertHostListRow (const char *, gint, Connection *);
void svLog (const char *, gboolean);
void svConnectionOpen (Connection *);
void svToLower (char *);
void svSavePreviousQuickNoteText (const Connection *);
void svServerError (VncConnection *, const char *, void *);
void svSetIconFromConnectionName (const char *, guint);
void svSetTextFromConnectionName (const char *, const char *);
void svSetToolsMenuItems(gboolean);
void svSetTooltip(GtkWidget *, const char *);
void svSetMenuItemTooltips ();
void svShowAboutWindow ();
void svShowAppOptionsWindow ();
void svShowConnectionActionsWindow ();
void svShowMessageDialog (const char *);
gpointer svSSHConnectionCloser (gpointer);
void svStartConnection (Connection *);
gboolean svStringToBool (const char *);

#endif
