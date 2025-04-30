/*
 * spiritvnc.c - 2022-2025 Will Brokenbourgh
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

#include "spiritvnc.h"


/* debug log since Windows doesn't print to console */
void svLog (char * strIn, bool skipStdOut)
{
  // make time string
  GDateTime * now = g_date_time_new_now_local();
  char * nowStr = g_date_time_format(now, "%Y-%m-%d-%H:%M:%S");

  // print to stdout if we aren't skipping or app->debugMode is true
  if (skipStdOut == false || app->debugMode == true)
  {
    // print to stdout
    printf("SpiritVNC-GTK: %s - %s\n", nowStr, strIn);
  }

  // only log to file if set in options
  if (app->logToFile == true)
  {
    FILE * f = fopen(app->appLogFile->str, "a");

    if (f == NULL)
      return;

    // print to file
    fprintf(f, "%s: %s\n", nowStr, strIn);

    fflush(f);
    fclose(f);
  }
}


/* convert char * to lower-case - in place */
void svToLower (char * strIn)
{
  if (strIn == NULL)
    return;

  int intLen = strlen(strIn);
  char strLow[intLen + 1];

  for (int i = 0; i < intLen; i ++)
    strLow[i] = tolower(strIn[i]);

  strIn[0] = '\0';

  strncpy(strIn, strLow, intLen);
}


/* convert string to bool */
bool svStringToBool (char * strIn)
{
  if (strIn == NULL)
    return false;

  int intLen = strlen(strIn);

  char strLow[intLen + 1];
  strncpy(strLow, strIn, sizeof(strLow));

  svToLower(strLow);

  if (strcmp(strLow, "1") == 0)
    return true;
  else if (strcmp(strLow, "true") == 0)
    return true;
  else if (strcmp(strLow, "t") == 0)
    return true;
  else if (strcmp(strLow, "yes") == 0)
    return true;
  else if (strcmp(strLow, "y") == 0)
    return true;

  return false;
}


/* return connection that matches the passed vncObj */
Connection * svConnectionFromVNCObj (GtkWidget * vncObj)
{
  Connection * con = NULL;

  // go through connection glist and return correct connection
  for (GList * l = app->connections; l != NULL; l = l->next)
  {
    con = (Connection *)l->data;

    if (con != NULL && con->vncObj == vncObj)
      return con;
  }

  return NULL;
}


/* return a zero (0) or one (1) from a bool */
/* (justification: some systems don't always return a '1' for true and '0' for false) */
unsigned int svIntFromBool (bool boolIn)
{
  if (boolIn == true)
    return 1;
  else
    return 0;
}


/* sets the passed variable with whatever text is selected in the connection list */
void svSelectedRowText (char * selectedRowText)
{
  GtkListBoxRow * selectedRow = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->serverList));

  if (selectedRow == NULL)
    return;

  // get box that holds status image and label
  GtkWidget * childBox = gtk_bin_get_child(GTK_BIN(selectedRow));

  if (childBox == NULL)
    return;

  // get box's children
  GList * boxChildren = gtk_container_get_children(GTK_CONTAINER(childBox));

  GtkWidget * label = g_list_nth_data(boxChildren, 1);

  if (label == NULL)
    return;

  // get label's text, if any
  const char * rText = gtk_label_get_text(GTK_LABEL(label)); //child));

  unsigned int textLen = strlen(rText);

  if (textLen < 1)
    return;

  char rowText[textLen + 1];
  memset(rowText, '\0', textLen + 1);
  strcpy(rowText, rText);

  strncpy(selectedRowText, rowText, textLen);
}


/* initialize app variables */
void svInitAppVars ()
{
  if (app == NULL)
  {
    printf("SpiritVNC-GTK: CRITICAL - app variable is NULL -- terminating app");
    exit(-1);
  }

  // important widgets
  app->mainWin = NULL;
  app->parent = NULL;
  app->serverList = NULL;
  app->toolsItems = (ToolsMenuItems *)malloc(sizeof(ToolsMenuItems));

  // important objects
  app->connections = NULL;
  app->selectedConnection = NULL;
  app->f12Storage = g_string_new(NULL);

  // app properties
  app->serverListWidth = 120;
  app->showTooltips = true;
  app->maximized = false;
  app->logToFile = false;
  app->debugMode = false;
  app->scanTimeout = 3;
  app->vncConnectWaitTime = 10;
  app->addNewConnection = true;

  // ssh stuff
  app->sshCommand = g_string_new(NULL);
  app->sshConnectWaitTime = 10;
  app->sshShowWaitTime = 3;

  //# flags, states and stuff
  //is_writing_config = False
  //listen_mode = False

  // important paths

  // app config dir
  app->appConfigDir = g_string_new(g_get_user_config_dir());
  g_string_append(app->appConfigDir, "/spiritvnc-gtk");

  // app config file
  app->appConfigFile = g_string_new(app->appConfigDir->str);
  g_string_append(app->appConfigFile, "/spiritvnc-gtk.conf");

  // app log file
  app->appLogFile = g_string_new(app->appConfigDir->str);
  g_string_append(app->appLogFile, "/spiritvnc-gtk.log");

  //# clipboard goodies
  //clip = Gtk.Clipboard()
  //server_clip = False
  //client_clip = False
}


/* initialize a connection object */
void svInitConnObject (Connection * con)
{
  if (con == NULL)
  {
    printf("con is null in svInitConnObject\n");
    return;
  }

  con->name = g_string_new(NULL);
  con->group = g_string_new(NULL);
  con->address = g_string_new(NULL);
  con->vncPort = g_string_new(NULL);
  con->vncPass = g_string_new(NULL);
  con->vncLoginUser = g_string_new(NULL);
  con->vncLoginPass = g_string_new(NULL);
  con->type = SV_TYPE_VNC;
  con->state = SV_STATE_DISCONNECTED;
  con->scale = false;
  con->lossyEncoding = false;
  con->showRemoteCursor = true;
  con->quality = SV_QUAL_DEFAULT;
  con->sshPort = g_string_new(NULL);
  con->sshPrivKeyfile = g_string_new(NULL);
  con->sshUser = g_string_new(NULL);
  con->sshPass = g_string_new(NULL);
  con->f12Macro = g_string_new(NULL);
  con->quickNote = g_string_new(NULL);
  con->customCmd1Enabled = false;
  con->customCmd1Label = g_string_new(NULL);
  con->customCmd1 = g_string_new(NULL);
  con->customCmd2Enabled = false;
  con->customCmd2Label = g_string_new(NULL);
  con->customCmd2 = g_string_new(NULL);
  con->customCmd3Enabled = false;
  con->customCmd3Label = g_string_new(NULL);
  con->customCmd3 = g_string_new(NULL);
  con->vncObj = NULL;
  con->disconnectType = SV_DISC_NONE;
  con->lastErrorMessage = g_string_new(NULL);
  con->lastConnectTime = g_string_new(NULL);
  con->sshThread = NULL;
  con->sshMonitorThread = NULL;
  con->sshReady = false;
  con->sshStream = NULL;
  con->clipboard = g_string_new(NULL);
}


/* set / unset hostlist items tooltips */
void svSetHostlistItemsTooltips ()
{
  GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

  // loop through each row
  for (GList * l = rows; l != NULL; l = l->next)
  {
    GtkWidget * row = GTK_WIDGET(l->data);

    // get box that holds status image and label
    GtkWidget * childBox = gtk_bin_get_child(GTK_BIN(row));

    if (childBox == NULL)
      continue;

    // get box's children
    GList * boxChildren = gtk_container_get_children(GTK_CONTAINER(childBox));

    GtkWidget * label = g_list_nth_data(boxChildren, 1);

    if (label != NULL && GTK_IS_LABEL(label))
    {
      const char * text = gtk_label_get_text(GTK_LABEL(label));

      // process if not null and not empty
      if (text != NULL && strcmp(text, "") != 0)
      {
        Connection * con = svConnectionFromName(text);

        if (con != NULL)
        {
          // tooltips are enabled, process
          if (app->showTooltips == true)
          {
            char tipStr[FILENAME_MAX] = {0};
            char typeStr[25] = {0};

            // set connection type
            if (con->type == SV_TYPE_VNC)
              strcpy(typeStr, "VNC");
            else if (con->type == SV_TYPE_VNC_OVER_SSH)
              strcpy(typeStr, "VNC over SSH");

            // compose tooltip markup
            snprintf(tipStr, FILENAME_MAX - 1, "<b>%s</b>\nType: %s\nAddress: %s\nLast connected: %s",
              con->name->str, typeStr, con->address->str, con->lastConnectTime->str);

            // set tooltip markup
            gtk_widget_set_tooltip_markup(childBox, tipStr);
          }
          else
            // unset tooltip
            gtk_widget_set_tooltip_text(childBox, NULL);
        }
      }
    }
  }
}


/* sets a widget or menu-item's tooltip */
void svSetTooltip(GtkWidget * widget, const char * text)
{
  if (widget == NULL || text == NULL)
    return;

  // set or unset tooltip, based on app options
  if (app->showTooltips == true)
    gtk_widget_set_tooltip_text(widget, text);
  else
    gtk_widget_set_tooltip_text(widget, NULL);
}


/* handle send entered keystrokes window 'cancel' button */
void svHandleSendEnteredKeystrokesCancel (GtkButton * self, gpointer userData)
{
  SendKeysObj * obj = (SendKeysObj *)userData;

  if (obj == NULL)
    return;

  // destroy send keystrokes window
  gtk_widget_destroy(obj->win);

  g_free(obj);
}


/* handle send entered keystrokes window 'ok' button */
void svHandleSendEnteredKeystrokesSend (GtkButton * self, gpointer userData)
{
  SendKeysObj * obj = (SendKeysObj *)userData;

  if (obj == NULL)
    return;

  // return if any of these are null
  if (obj->win == NULL || obj->textView == NULL || obj->con == NULL || obj->con->vncObj == NULL)
    return;

  // get textview buffer
  GtkTextBuffer * tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(obj->textView));

  // beginning and end iters
  GtkTextIter start;
  GtkTextIter end;

  // set iters
  gtk_text_buffer_get_start_iter(tb, &start);
  gtk_text_buffer_get_end_iter(tb, &end);

  // copy text to char array
  char sendText[FILENAME_MAX] = {0};
  strncpy(sendText, gtk_text_buffer_get_text(tb, &start, &end, FALSE), FILENAME_MAX - 1);

  // don't send an empty string
  if (strcmp(sendText, "") == 0)
    return;

  unsigned int keys[strlen(sendText) + 1];
  memset(keys, '\0', sizeof(keys));

  // convert text to uint array
  for (size_t i = 0; i < strlen(sendText); i++)
    keys[i] = gdk_unicode_to_keyval(sendText[i]);

  // send the keys to server
  vnc_display_send_keys(VNC_DISPLAY(obj->con->vncObj), keys, sizeof(keys)/sizeof(keys[0]));

  // destroy the send keystroke window
  gtk_widget_destroy(obj->win);

  g_free(obj);
}


/* handle custom command output window 'ok' button */
void svHandleCommandOutputOk (GtkButton * self, gpointer userData)
{
  GtkWidget * outWin = (GtkWidget *)userData;

  if (outWin == NULL)
    return;

  // destroy the command output window
  gtk_widget_destroy(outWin);
}


/* handle the app options 'cancel' button */
void svHandleAppOptionsCancel (GtkButton * self, gpointer userData)
{
  AppOptions * opts = (AppOptions *)userData;

  if (opts == NULL)
    return;

  GtkWidget * optsWin = (GtkWidget *)opts->optionsWin;

  if (optsWin == NULL)
    return;

  // log
  svLog("Canceled app options editing", true);

  // destroy the app options window
  gtk_widget_destroy(optsWin);

  g_free(opts);
}


/* handle the app options 'save' button */
void svHandleAppOptionsSave (GtkButton * self, gpointer userData)
{
  AppOptions * opts = (AppOptions *)userData;

  if (opts == NULL)
    return;

  GtkWidget * optsWin = (GtkWidget *)opts->optionsWin;

  if (optsWin == NULL)
    return;

  // log
  svLog("Saving app options", true);

  // -------------------------------------------

  // show tooltips
  app->showTooltips = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(opts->showTooltips));

  // log to file
  app->logToFile = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(opts->logToFile));

  // scan timeout
  app->scanTimeout = atoi(gtk_entry_get_text(GTK_ENTRY(opts->scanTimeout)));

  // vnc connect wait time
  app->vncConnectWaitTime = atoi(gtk_entry_get_text(GTK_ENTRY(opts->vncWaitTime)));

  // ssh connect wait time
  app->sshConnectWaitTime = atoi(gtk_entry_get_text(GTK_ENTRY(opts->sshWaitTime)));

  // ssh command
  g_string_assign(app->sshCommand, gtk_entry_get_text(GTK_ENTRY(opts->sshCommand)));

  // -------------------------------------------

  // set or unset tooltips
  svSetMenuItemTooltips();
  svSetHostlistItemsTooltips();

  // write out our config
  svConfigWrite();

  // destroy options window
  gtk_widget_destroy(opts->optionsWin);

  g_free(opts);
}


/* handle the connection edit 'save' button */
void svHandleConnectionSettingsSave (GtkButton * self, gpointer userData)
{
  ConnectionSettings * settings = (ConnectionSettings *)userData;

  if (settings == NULL || settings->settingsWin == NULL)
    return;

  // set con object
  Connection * con = settings->con;

  if (con == NULL)
    return;

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Saving settings for '%s - %s'", con->name->str, con->address->str);
  svLog(logStr, true);

  // connection name
  char oldVal[FILENAME_MAX] = {0};

  strncpy(oldVal, con->name->str, FILENAME_MAX - 1);

  char * nameVal = (char *)gtk_entry_get_text(GTK_ENTRY(settings->connectionName));

  // set new value
  //if (strcmp(nameVal, "") != 0)
  g_string_assign(con->name, nameVal);

  // update existing list item, if name isn't empty and has changed
  if (strcmp(nameVal, "") != 0)
  {
    // if value changed, find server list row with old name and update
    if (strcmp(oldVal, nameVal) != 0)
    {
      GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

      // loop through each row
      for (GList * l = rows; l != NULL; l = l->next)
      {
        GtkWidget * row = GTK_WIDGET(l->data);

        // get box that holds status image and label
        GtkWidget * childBox = gtk_bin_get_child(GTK_BIN(row));

        if (childBox == NULL)
          continue;

        // get box's children
        GList * boxChildren = gtk_container_get_children(GTK_CONTAINER(childBox));

        GtkWidget * label = g_list_nth_data(boxChildren, 1);

        if (label != NULL && GTK_IS_LABEL(label))
        {
          const char * text = gtk_label_get_text(GTK_LABEL(label));

          if (strcmp(text, oldVal) == 0)
          {
            gtk_label_set_text(GTK_LABEL(label), nameVal);
            break;
          }
        }
      }

      // Free the list
      g_list_free(rows);
    }
  }

  // connection group
  char * groupVal = (char *)gtk_entry_get_text(GTK_ENTRY(settings->connectionGroup));

  // set new value
  if (strcmp(groupVal, "") != 0)
    g_string_assign(con->group, groupVal);
  else
    g_string_assign(con->group, "General");

  // connection address
  char * addrVal = (char *)gtk_entry_get_text(GTK_ENTRY(settings->remoteAddress));

  // set new value
  //if (strcmp(addrVal, "") != 0)
  g_string_assign(con->address, addrVal);

  // connection f12 macro
  g_string_assign(con->f12Macro, gtk_entry_get_text(GTK_ENTRY(settings->f12Macro)));

  // vnc choice radio button
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->vncChoice)) == true)
    con->type = SV_TYPE_VNC;

  // vnc-over-ssh choice radio button
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->svncChoice)) == true)
    con->type = SV_TYPE_VNC_OVER_SSH;

  // vnc port
  char * vPortVal = (char *)gtk_entry_get_text(GTK_ENTRY(settings->vncPort));

  // set new value
  //if (strcmp(vPortVal, "") != 0)
  g_string_assign(con->vncPort, vPortVal);

  // vnc password
  const char * vPassVal = gtk_entry_get_text(GTK_ENTRY(settings->vncPassword));

  g_string_assign(con->vncPass, g_base64_encode((const unsigned char *)vPassVal, strlen(vPassVal)));

  // vnc 'login' username
  g_string_assign(con->vncLoginUser, gtk_entry_get_text(GTK_ENTRY(settings->vncLoginUsername)));

  // vnc 'login' password
  const char * vLoginPassVal = gtk_entry_get_text(GTK_ENTRY(settings->vncLoginPassword));

  g_string_assign(con->vncLoginPass, g_base64_encode((const unsigned char *)vLoginPassVal, strlen(vLoginPassVal)));

  // vnc image quality
  con->quality = gtk_combo_box_get_active(GTK_COMBO_BOX(settings->vncQuality));

  // vnc lossy encoding
  con->lossyEncoding = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->vncLossyEncoding));

  // vnc scaling
  con->scale = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->vncScaling));

  // ssh username
  g_string_assign(con->sshUser, gtk_entry_get_text(GTK_ENTRY(settings->sshUsername)));

  // ssh port
  g_string_assign(con->sshPort, gtk_entry_get_text(GTK_ENTRY(settings->sshPort)));

  // ssh private key file
  g_string_assign(con->sshPrivKeyfile, gtk_entry_get_text(GTK_ENTRY(settings->sshPrivateKey)));

  // custom command 1 enabled
  con->customCmd1Enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->cmd1Enabled));

  // custom command 1 label
  g_string_assign(con->customCmd1Label, gtk_entry_get_text(GTK_ENTRY(settings->cmd1Label)));

  // custom command 1
  g_string_assign(con->customCmd1, gtk_entry_get_text(GTK_ENTRY(settings->cmd1)));

  // custom command 2 enabled
  con->customCmd2Enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->cmd2Enabled));

  // custom command 2 label
  g_string_assign(con->customCmd2Label, gtk_entry_get_text(GTK_ENTRY(settings->cmd2Label)));

  // custom command 2
  g_string_assign(con->customCmd2, gtk_entry_get_text(GTK_ENTRY(settings->cmd2)));

  // custom command 3 enabled
  con->customCmd3Enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->cmd3Enabled));

  // custom command 3 label
  g_string_assign(con->customCmd3Label, gtk_entry_get_text(GTK_ENTRY(settings->cmd3Label)));

  // custom command 3
  g_string_assign(con->customCmd3, gtk_entry_get_text(GTK_ENTRY(settings->cmd3)));

  // write out the settings
  svConfigWrite();

  // ========= add to the hostlist and connections if this is a new connection =========
  if (app->addNewConnection == true) // && strcmp(con->address->str, "") != 0)
  {
    // if no connection name, warn user
    if (strcmp(con->name->str, "") == 0)
    {
      GtkWidget * dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(settings->settingsWin),
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_ERROR,
                                      GTK_BUTTONS_CLOSE,
                                      "<b>Error: 'Connection name' cannot be empty</b>\n\nPlease enter a "
                                        "connection name then try saving again");
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);

      // focus remote address
      gtk_widget_grab_focus(settings->connectionName);

      return;
    }

    // if no host address, warn user
    if (strcmp(con->address->str, "") == 0)
    {
      GtkWidget * dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(settings->settingsWin),
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_ERROR,
                                      GTK_BUTTONS_CLOSE,
                                      "<b>Error: 'Remote Address' cannot be empty</b>\n\nPlease enter a "
                                        "valid remote address then try saving again");
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);

      // focus remote address
      gtk_widget_grab_focus(settings->remoteAddress);

      return;
    }

    // check if a connection exists with the same name
    Connection * conTemp = svConnectionFromName(con->name->str);

    // if a connection exists with this name, warn user
    if (conTemp != NULL)
    {
      GtkWidget * dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(settings->settingsWin),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        "<b>Error: Connection name already exists</b>\n\nPlease change the "
                                          "connection's name then try saving again");
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);

      // focus the connection name
      gtk_widget_grab_focus(settings->connectionName);

      return;
    }

    // get currently selected row (if any)
    GtkListBoxRow * row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->serverList));

    int rowIdx = -1;

    // get currently selected connection list row
    if (row != NULL)
      rowIdx = gtk_list_box_row_get_index(row);

    // if we're at the empty first list row, find the last of this connection's
    // group and insert afterward
    if (rowIdx == 0)
    {
      rowIdx = -1;
      char * lastGroup = NULL;

      // go through connections glist and find last of this group
      for (GList * l = app->connections; l != NULL; l = l->next)
      {
        conTemp = (Connection *)l->data;

        // break if we are at the end of this new connection's group
        if (conTemp != NULL)
        {
          if (lastGroup != NULL
            && strcmp(conTemp->group->str, lastGroup) != 0
            && strcmp(lastGroup, con->group->str) == 0)
            break;

          lastGroup = conTemp->group->str;
        }

        rowIdx++;
      }
    }
    else
    {
      // add 1 if the currently selected row is not -1
      if (rowIdx != -1)
        rowIdx++;
    }

    // insert connection to connections GList
    app->connections = g_list_insert(app->connections, con, rowIdx);

    // insert into connections list at currently selected location or
    // the end of the list if rowIdx is -1
    svInsertHostListRow(con->name->str, rowIdx);

    app->addNewConnection = false;

    svSetHostlistItemsTooltips();
  }

  // destroy the connection settings window
  gtk_widget_destroy(settings->settingsWin);

  // free the malloc'd settings
  g_free(settings);
}


/* handles the connection editor cancel button */
void svHandleConnectionSettingsCancel (GtkButton* self, gpointer userData)
{
  ConnectionSettings * settings = (ConnectionSettings *)userData;

  if (settings == NULL || settings->settingsWin == NULL)
    return;

  // log
  svLog("Canceled connection editing", true);

  // destroy connection editing window
  gtk_widget_destroy(settings->settingsWin);

  // free the malloc'd settings
  g_free(settings);
}


/* handles the 'choose private key' button in the connection editor */
void svHandleConnectionSSHPrivKey (GtkButton * self, gpointer userData)
{
  ConnectionSettings * settings = (ConnectionSettings *)userData;

  if (settings == NULL || settings->settingsWin == NULL)
    return;

  GtkWidget * entry = settings->sshPrivateKey;

  if (entry == NULL || GTK_IS_ENTRY(entry) == false)
    return;

  GtkWidget * dialog;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  int res;

  dialog = gtk_file_chooser_dialog_new("Choose a private key",
                                        GTK_WINDOW(settings->settingsWin),
                                        action,
                                        ("_Cancel"),
                                        GTK_RESPONSE_CANCEL,
                                        ("_Open"),
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);

  res = gtk_dialog_run(GTK_DIALOG (dialog));

  if (res == GTK_RESPONSE_ACCEPT)
  {
    char * filename;
    GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);
    filename = gtk_file_chooser_get_filename(chooser);
    gtk_entry_set_text(GTK_ENTRY(entry), filename);
    g_free(filename);
  }

  // destroy the dialog window
  gtk_widget_destroy(dialog);
}


/* create and display connection edit window */
void svShowAppOptionsWindow ()
{
  AppOptions * opts = (AppOptions *)malloc(sizeof(AppOptions));

  if (opts == NULL)
    return;

  // log
  svLog("Editing app options", true);

  // create edit window
  opts->optionsWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  // set window title
  gtk_window_set_title(GTK_WINDOW(opts->optionsWin), "SpiritVNC App Options");
  gtk_window_set_modal(GTK_WINDOW(opts->optionsWin), true);
  gtk_window_set_resizable(GTK_WINDOW(opts->optionsWin), false);

  // parent box
  GtkWidget * boxOptsParent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
  gtk_container_set_border_width(GTK_CONTAINER(boxOptsParent), 5);
  gtk_container_add(GTK_CONTAINER(opts->optionsWin), boxOptsParent);

  // container grid
  GtkWidget * optsPage = gtk_grid_new();
  gtk_grid_set_column_homogeneous(GTK_GRID(optsPage), false);
  gtk_container_set_border_width(GTK_CONTAINER(optsPage), 7);
  gtk_grid_set_column_spacing(GTK_GRID(optsPage), 7);
  gtk_grid_set_row_spacing(GTK_GRID(optsPage), 5);

  unsigned int rowNum = 1;

  // show tooltips
  GtkWidget * lblTooltips = gtk_label_new("Show tooltips");
  gtk_widget_set_halign(lblTooltips, GTK_ALIGN_END);
  opts->showTooltips = gtk_check_button_new();
  if (app->showTooltips == true)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(opts->showTooltips), true);
  svSetTooltip(opts->showTooltips, "Displays tooltips for most app items");

  gtk_grid_attach(GTK_GRID(optsPage), lblTooltips, 1, rowNum, 1, 1);
  gtk_grid_attach(GTK_GRID(optsPage), opts->showTooltips, 2, rowNum++, 1, 1);

  // log to file
  GtkWidget * lblLog = gtk_label_new("Log events to file");
  gtk_widget_set_halign(lblLog, GTK_ALIGN_END);
  opts->logToFile = gtk_check_button_new();
  svSetTooltip(opts->logToFile, "Logs most app events to a log file");

  if (app->logToFile == true)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(opts->logToFile), true);

  gtk_grid_attach(GTK_GRID(optsPage), lblLog, 1, rowNum, 1, 1);
  gtk_grid_attach(GTK_GRID(optsPage), opts->logToFile, 2, rowNum++, 1, 1);

  // scan timeout
  GtkWidget * lblScanTime = gtk_label_new("Scan timeout (secs)");
  gtk_widget_set_halign(lblScanTime, GTK_ALIGN_END);
  opts->scanTimeout = gtk_spin_button_new_with_range(0, 100, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(opts->scanTimeout), app->scanTimeout);
  svSetTooltip(opts->scanTimeout, "How long the app waits before moving to the next viewer "
    "when scanning is enabled");

  gtk_grid_attach(GTK_GRID(optsPage), lblScanTime, 1, rowNum, 1, 1);
  gtk_grid_attach(GTK_GRID(optsPage), opts->scanTimeout, 2, rowNum++, 1, 1);

  // vnc wait time
  GtkWidget * lblVNCWaitTime = gtk_label_new("VNC wait time");
  gtk_widget_set_halign(lblVNCWaitTime, GTK_ALIGN_END);
  opts->vncWaitTime = gtk_spin_button_new_with_range(0, 100, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(opts->vncWaitTime), app->vncConnectWaitTime);
  svSetTooltip(opts->vncWaitTime, "How long the app waits before a VNC connection "
    "attempt times out");

  gtk_grid_attach(GTK_GRID(optsPage), lblVNCWaitTime, 1, rowNum, 1, 1);
  gtk_grid_attach(GTK_GRID(optsPage), opts->vncWaitTime, 2, rowNum++, 1, 1);

  // ssh wait time
  GtkWidget * lblSSHWaitTime = gtk_label_new("SSH wait time");
  gtk_widget_set_halign(lblSSHWaitTime, GTK_ALIGN_END);
  opts->sshWaitTime = gtk_spin_button_new_with_range(0, 100, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(opts->sshWaitTime), app->sshConnectWaitTime);
  svSetTooltip(opts->sshWaitTime, "How long the app waits before a SSH connection "
    "attempt times out");

  gtk_grid_attach(GTK_GRID(optsPage), lblSSHWaitTime, 1, rowNum, 1, 1);
  gtk_grid_attach(GTK_GRID(optsPage), opts->sshWaitTime, 2, rowNum++, 1, 1);

  // ssh command
  GtkWidget * lblSSHCmd = gtk_label_new("SSH command");
  gtk_widget_set_halign(lblSSHCmd, GTK_ALIGN_END);
  opts->sshCommand = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(opts->sshCommand), 30);
  gtk_entry_set_text(GTK_ENTRY(opts->sshCommand), app->sshCommand->str);
  svSetTooltip(opts->sshCommand, "The system SSH command (/usr/bin/ssh, etc)");

  gtk_grid_attach(GTK_GRID(optsPage), lblSSHCmd, 1, rowNum, 1, 1);
  gtk_grid_attach(GTK_GRID(optsPage), opts->sshCommand, 2, rowNum++, 1, 1);

  // add optsPage to parent box
  gtk_box_pack_start(GTK_BOX(boxOptsParent), optsPage, false, false, 0);

  // ------------------- save / cancel buttons -------------------
  GtkWidget * boxButtons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  // save
  GtkWidget * btnSave = gtk_button_new_with_label("Save");
  gtk_widget_set_size_request(btnSave, 110, -1);
  svSetTooltip(btnSave, "Click to save these settings");
  g_signal_connect(btnSave, "clicked", G_CALLBACK(svHandleAppOptionsSave), opts);

  // cancel
  GtkWidget * btnCancel = gtk_button_new_with_label("Cancel");
  gtk_widget_set_size_request(btnCancel, 110, -1);
  svSetTooltip(btnCancel, "Click to abandon any changes.  "
    "Caution: Any changes will be lost!");
  g_signal_connect(btnCancel, "clicked", G_CALLBACK(svHandleAppOptionsCancel), opts);

  gtk_box_pack_end(GTK_BOX(boxButtons), btnSave, false, false, 3);
  gtk_box_pack_end(GTK_BOX(boxButtons), btnCancel, false, false, 3);
  //
  gtk_container_add(GTK_CONTAINER(boxOptsParent), boxButtons);

  // show everything
  gtk_widget_show_all(opts->optionsWin);

  // focus the first entry
  gtk_widget_grab_focus(opts->showTooltips);
}


/* create and display connection edit window */
void svShowConnectionEditWindow (Connection * con)
{
  if (con == NULL || strcmp(con->name->str, "") == 0)
    return;

  ConnectionSettings * settings = (ConnectionSettings *)malloc(sizeof(ConnectionSettings));

  if (settings == NULL)
    return;

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Editing connection '%s - %s'", con->name->str, con->address->str);
  svLog(logStr, true);

  settings->con = con;

  // create edit window
  settings->settingsWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_modal(GTK_WINDOW(settings->settingsWin), true);
  gtk_window_set_resizable(GTK_WINDOW(settings->settingsWin), false);

  // set window title
  char strTitle[1024] = {0};
  snprintf(strTitle, 1023, "Editing '%s' - SpiritVNC", con->name->str);
  gtk_window_set_title(GTK_WINDOW(settings->settingsWin), strTitle);

  // create parent box
  GtkWidget * boxEditParent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
  gtk_container_set_border_width(GTK_CONTAINER(boxEditParent), 5);
  gtk_container_add(GTK_CONTAINER(settings->settingsWin), boxEditParent);

  // tab control
  GtkWidget * editTab = gtk_notebook_new();

  // ------------------- vnc page -------------------
  GtkWidget * vncPage = gtk_grid_new();
  GtkWidget * lblVNCTab = gtk_label_new("VNC options");
  gtk_container_set_border_width(GTK_CONTAINER(vncPage), 7);
  gtk_grid_set_column_spacing(GTK_GRID(vncPage), 7);
  gtk_grid_set_row_spacing(GTK_GRID(vncPage), 5);

  // connection name
  GtkWidget * lblConName = gtk_label_new("Connection name");
  gtk_widget_set_halign(lblConName, GTK_ALIGN_END);
  settings->connectionName = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->connectionName), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->connectionName), con->name->str);
  svSetTooltip(settings->connectionName, "The name you choose for this connection "
    "(not the host address)");

  gtk_grid_attach(GTK_GRID(vncPage), lblConName, 1, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->connectionName, 2, 1, 3, 1);

  // connection group
  GtkWidget * lblConGroup = gtk_label_new("Connection group");
  gtk_widget_set_halign(lblConGroup, GTK_ALIGN_END);
  settings->connectionGroup = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->connectionGroup), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->connectionGroup), con->group->str);
  svSetTooltip(settings->connectionGroup, "The group this connection belongs to");

  gtk_grid_attach(GTK_GRID(vncPage), lblConGroup, 1, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->connectionGroup, 2, 2, 3, 1);

  // connection address
  GtkWidget * lblConAddr = gtk_label_new("Remote address");
  gtk_widget_set_halign(lblConAddr, GTK_ALIGN_END);
  settings->remoteAddress = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->remoteAddress), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->remoteAddress), con->address->str);
  svSetTooltip(settings->remoteAddress, "The network address of the remote host");

  gtk_grid_attach(GTK_GRID(vncPage), lblConAddr, 1, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->remoteAddress, 2, 3, 3, 1);

  // f12 macro
  GtkWidget * lblF12Macro = gtk_label_new("F12 macro");
  gtk_widget_set_halign(lblF12Macro, GTK_ALIGN_END);
  settings->f12Macro = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->f12Macro), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->f12Macro), con->f12Macro->str);
  svSetTooltip(settings->f12Macro, "The characters sent to the remote host when the "
    "F12 key is pressed");

  gtk_grid_attach(GTK_GRID(vncPage), lblF12Macro, 1, 4, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->f12Macro, 2, 4, 3, 1);

  // vnc or vnc-over-ssh
  GtkWidget * boxVNCChoice = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);

  GtkWidget * lblVNCChoice = gtk_label_new("Server type");
  gtk_widget_set_halign(lblVNCChoice, GTK_ALIGN_END);

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCChoice, 1, 5, 1, 1);

  // - vnc choice group
  GSList * vncChoiceGroup = NULL;
  settings->vncChoice = gtk_radio_button_new_with_label(vncChoiceGroup, "VNC");
  vncChoiceGroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(settings->vncChoice));
  settings->svncChoice = gtk_radio_button_new_with_label(vncChoiceGroup, "VNC over SSH");
  svSetTooltip(settings->vncChoice, "Sets this to be a VNC connection");
  vncChoiceGroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(settings->svncChoice));
  svSetTooltip(settings->svncChoice, "Sets this to be a VNC-over-SSH connection");

  // - set value from connection
  if (con->type == SV_TYPE_VNC)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->vncChoice), true);
  else if (con->type == SV_TYPE_VNC_OVER_SSH)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->svncChoice), true);

  gtk_container_add(GTK_CONTAINER(boxVNCChoice), settings->vncChoice);
  gtk_container_add(GTK_CONTAINER(boxVNCChoice), settings->svncChoice);

  gtk_grid_attach(GTK_GRID(vncPage), boxVNCChoice, 2, 5, 1, 1);

  // vnc port
  GtkWidget * lblVNCPort = gtk_label_new("VNC port");
  gtk_widget_set_halign(lblVNCPort, GTK_ALIGN_END);
  settings->vncPort = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->vncPort), 10);
  gtk_entry_set_text(GTK_ENTRY(settings->vncPort), con->vncPort->str);
  svSetTooltip(settings->vncPort, "The remote host's numbered VNC port");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCPort, 1, 6, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncPort, 2, 6, 3, 1);

  // vnc password
  GtkWidget * lblVNCPass = gtk_label_new("VNC password");
  gtk_widget_set_halign(lblVNCPass, GTK_ALIGN_END);
  settings->vncPassword = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->vncPassword), 30);
  gtk_entry_set_visibility(GTK_ENTRY(settings->vncPassword), false);
  gsize vncPassOutLen = 0;
  unsigned char * vncPassTemp = g_base64_decode(con->vncPass->str, &vncPassOutLen);
  gtk_entry_set_text(GTK_ENTRY(settings->vncPassword), (const char *)vncPassTemp);
  svSetTooltip(settings->vncPassword, "The standard VNC password");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCPass, 1, 7, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncPassword, 2, 7, 3, 1);

  // vnc login username
  GtkWidget * lblVNCLoginName = gtk_label_new("VNC login username");
  gtk_widget_set_halign(lblVNCLoginName, GTK_ALIGN_END);
  settings->vncLoginUsername = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->vncLoginUsername), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->vncLoginUsername), con->vncLoginUser->str);
  svSetTooltip(settings->vncLoginUsername, "The username used with VNC 'login' authentication");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCLoginName, 1, 8, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncLoginUsername, 2, 8, 3, 1);

  // vnc login password
  GtkWidget * lblVNCLoginPass = gtk_label_new("VNC login password");
  gtk_widget_set_halign(lblVNCLoginPass, GTK_ALIGN_END);
  settings->vncLoginPassword = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->vncLoginPassword), 30);
  gtk_entry_set_visibility(GTK_ENTRY(settings->vncLoginPassword), false);
  gsize vncLoginPassOutLen = 0;
  unsigned char * vncLoginPassTemp = g_base64_decode(con->vncLoginPass->str, &vncLoginPassOutLen);
  gtk_entry_set_text(GTK_ENTRY(settings->vncLoginPassword), (const char *)vncLoginPassTemp);
  svSetTooltip(settings->vncLoginPassword, "The password used with VNC 'login' authentication");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCLoginPass, 1, 9, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncLoginPassword, 2, 9, 3, 1);

  // vnc quality
  GtkWidget * lblVNCQuality = gtk_label_new("VNC quality");
  gtk_widget_set_halign(lblVNCQuality, GTK_ALIGN_END);
  settings->vncQuality = gtk_combo_box_text_new();

  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(settings->vncQuality), "Low");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(settings->vncQuality), "Medium");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(settings->vncQuality), "Full");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(settings->vncQuality), "Default");

  gtk_combo_box_set_active(GTK_COMBO_BOX(settings->vncQuality), con->quality);
  svSetTooltip(settings->vncQuality, "The remote host's displayed image quality");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCQuality, 1, 10, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncQuality, 2, 10, 3, 1);

  // vnc lossy encoding
  GtkWidget * lblVNCLossy = gtk_label_new("VNC lossy encoding");
  gtk_widget_set_halign(lblVNCLossy, GTK_ALIGN_END);
  settings->vncLossyEncoding = gtk_check_button_new();
  if (con->lossyEncoding == true)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->vncLossyEncoding), true);
  svSetTooltip(settings->vncLossyEncoding, "Enables the remote host's lossy encoding");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCLossy, 1, 11, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncLossyEncoding, 2, 11, 3, 1);

  // vnc scaling
  GtkWidget * lblVNCScaling = gtk_label_new("VNC scaling");
  gtk_widget_set_halign(lblVNCScaling, GTK_ALIGN_END);
  settings->vncScaling = gtk_check_button_new();
  if (con->scale == true)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->vncScaling), true);
  svSetTooltip(settings->vncScaling, "Scales the remote host's display");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCScaling, 1, 12, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncScaling, 2, 12, 3, 1);

  //
  gtk_notebook_append_page(GTK_NOTEBOOK(editTab), vncPage, lblVNCTab);

  // ------------------- ssh page -------------------
  GtkWidget * sshPage = gtk_grid_new();
  GtkWidget * lblSSHTab = gtk_label_new("SSH options");
  gtk_container_set_border_width(GTK_CONTAINER(sshPage), 7);
  gtk_grid_set_column_spacing(GTK_GRID(sshPage), 7);
  gtk_grid_set_row_spacing(GTK_GRID(sshPage), 5);

  // ssh name
  GtkWidget * lblSSHName = gtk_label_new("SSH username");
  gtk_widget_set_halign(lblSSHName, GTK_ALIGN_END);
  settings->sshUsername = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->sshUsername), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->sshUsername), con->sshUser->str);
  svSetTooltip(settings->sshUsername, "The username used for the remote host's SSH server");

  gtk_grid_attach(GTK_GRID(sshPage), lblSSHName, 1, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(sshPage), settings->sshUsername, 2, 1, 3, 1);

  // ssh port
  GtkWidget * lblSSHPort = gtk_label_new("SSH port");
  gtk_widget_set_halign(lblSSHPort, GTK_ALIGN_END);
  settings->sshPort = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->sshPort), 10);
  gtk_entry_set_text(GTK_ENTRY(settings->sshPort), con->sshPort->str);
  svSetTooltip(settings->sshPort, "The remote host's numbered SSH server's port");

  gtk_grid_attach(GTK_GRID(sshPage), lblSSHPort, 1, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(sshPage), settings->sshPort, 2, 2, 3, 1);

  // ssh private key
  GtkWidget * lblSSHPrivKey = gtk_label_new("SSH private key (if any)");
  gtk_widget_set_halign(lblSSHPrivKey, GTK_ALIGN_END);
  settings->sshPrivateKey = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->sshPrivateKey), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->sshPrivateKey), con->sshPrivKeyfile->str);
  svSetTooltip(settings->sshPrivateKey, "The private key (if any) used to authenticate on the remote "
    "host's SSH server");

  GtkWidget * btnSSHPrivKey = gtk_button_new_with_label("...");
  g_signal_connect(btnSSHPrivKey, "clicked", G_CALLBACK(svHandleConnectionSSHPrivKey), settings->sshPrivateKey);
  svSetTooltip(btnSSHPrivKey, "Click to select a private key for the remote host's SSH server");

  gtk_grid_attach(GTK_GRID(sshPage), lblSSHPrivKey, 1, 4, 1, 1);
  gtk_grid_attach(GTK_GRID(sshPage), settings->sshPrivateKey, 2, 4, 1, 1);
  gtk_grid_attach(GTK_GRID(sshPage), btnSSHPrivKey, 3, 4, 1, 1);

  //
  gtk_notebook_append_page(GTK_NOTEBOOK(editTab), sshPage, lblSSHTab);

  // ------------------- custom commands page -------------------
  GtkWidget * customCmdsPage = gtk_grid_new();
  GtkWidget * lblCmdsTab = gtk_label_new("Custom commands");
  gtk_container_set_border_width(GTK_CONTAINER(customCmdsPage), 7);
  gtk_grid_set_column_spacing(GTK_GRID(customCmdsPage), 7);
  gtk_grid_set_row_spacing(GTK_GRID(customCmdsPage), 14);

  // command 1
  GtkWidget * frameCmd1 = gtk_frame_new("Command 1");
  gtk_frame_set_shadow_type(GTK_FRAME(frameCmd1), GTK_SHADOW_ETCHED_IN);
  gtk_grid_attach(GTK_GRID(customCmdsPage), frameCmd1, 1, 1, 1, 1);
  GtkWidget * gridCmd1 = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(gridCmd1), 5);
  gtk_grid_set_row_spacing(GTK_GRID(gridCmd1), 5);
  gtk_container_add(GTK_CONTAINER(frameCmd1), gridCmd1);

  // - enabled
  GtkWidget * lblCmd1Enabled = gtk_label_new("Enabled");
  gtk_widget_set_halign(lblCmd1Enabled, GTK_ALIGN_END);
  settings->cmd1Enabled = gtk_check_button_new();
  if (con->customCmd1Enabled == true)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->cmd1Enabled), true);
  svSetTooltip(settings->cmd1Enabled, "Enables custom command 1");

  gtk_grid_attach(GTK_GRID(gridCmd1), lblCmd1Enabled, 1, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(gridCmd1), settings->cmd1Enabled, 2, 2, 1, 1);

  // - label
  GtkWidget * lblCmd1Label = gtk_label_new("Label");
  gtk_widget_set_halign(lblCmd1Label, GTK_ALIGN_END);
  settings->cmd1Label = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->cmd1Label), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->cmd1Label), con->customCmd1Label->str);
  svSetTooltip(settings->cmd1Label, "The displayed menu label for custom command 1");

  gtk_grid_attach(GTK_GRID(gridCmd1), lblCmd1Label, 1, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(gridCmd1), settings->cmd1Label, 2, 3, 1, 1);

  // - command
  GtkWidget * lblCmd1 = gtk_label_new("Command");
  gtk_widget_set_halign(lblCmd1, GTK_ALIGN_END);
  settings->cmd1 = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->cmd1), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->cmd1), con->customCmd1->str);
  svSetTooltip(settings->cmd1, "The actual command that is run");

  gtk_grid_attach(GTK_GRID(gridCmd1), lblCmd1, 1, 4, 1, 1);
  gtk_grid_attach(GTK_GRID(gridCmd1), settings->cmd1, 2, 4, 1, 1);

  // command 2
  GtkWidget * frameCmd2 = gtk_frame_new("Command 2");
  gtk_frame_set_shadow_type(GTK_FRAME(frameCmd2), GTK_SHADOW_ETCHED_IN);
  gtk_grid_attach(GTK_GRID(customCmdsPage), frameCmd2, 1, 2, 1, 1);
  GtkWidget * gridCmd2 = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(gridCmd2), 5);
  gtk_grid_set_row_spacing(GTK_GRID(gridCmd2), 5);
  gtk_container_add(GTK_CONTAINER(frameCmd2), gridCmd2);

  // - enabled
  GtkWidget * lblCmd2Enabled = gtk_label_new("Enabled");
  gtk_widget_set_halign(lblCmd2Enabled, GTK_ALIGN_END);
  settings->cmd2Enabled = gtk_check_button_new();
  if (con->customCmd2Enabled == true)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->cmd2Enabled), true);
  svSetTooltip(settings->cmd2Enabled, "Enables custom command 2");

  gtk_grid_attach(GTK_GRID(gridCmd2), lblCmd2Enabled, 1, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(gridCmd2), settings->cmd2Enabled, 2, 2, 1, 1);

  // - label
  GtkWidget * lblCmd2Label = gtk_label_new("Label");
  gtk_widget_set_halign(lblCmd2Label, GTK_ALIGN_END);
  settings->cmd2Label = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->cmd2Label), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->cmd2Label), con->customCmd2Label->str);
  svSetTooltip(settings->cmd2Label, "The displayed menu label for custom command 2");

  gtk_grid_attach(GTK_GRID(gridCmd2), lblCmd2Label, 1, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(gridCmd2), settings->cmd2Label, 2, 3, 1, 1);

  // - command
  GtkWidget * lblCmd2 = gtk_label_new("Command");
  gtk_widget_set_halign(lblCmd2, GTK_ALIGN_END);
  settings->cmd2 = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->cmd2), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->cmd2), con->customCmd2->str);
  svSetTooltip(settings->cmd2, "The actual command that is run");

  gtk_grid_attach(GTK_GRID(gridCmd2), lblCmd2, 1, 4, 1, 1);
  gtk_grid_attach(GTK_GRID(gridCmd2), settings->cmd2, 2, 4, 1, 1);

  // command 3
  GtkWidget * frameCmd3 = gtk_frame_new("Command 3");
  gtk_frame_set_shadow_type(GTK_FRAME(frameCmd3), GTK_SHADOW_ETCHED_IN);
  gtk_grid_attach(GTK_GRID(customCmdsPage), frameCmd3, 1, 3, 1, 1);
  GtkWidget * gridCmd3 = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(gridCmd3), 5);
  gtk_grid_set_row_spacing(GTK_GRID(gridCmd3), 5);
  gtk_container_add(GTK_CONTAINER(frameCmd3), gridCmd3);

  // - enabled
  GtkWidget * lblCmd3Enabled = gtk_label_new("Enabled");
  gtk_widget_set_halign(lblCmd3Enabled, GTK_ALIGN_END);
  settings->cmd3Enabled = gtk_check_button_new();
  if (con->customCmd3Enabled == true)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->cmd3Enabled), true);
  svSetTooltip(settings->cmd3Enabled, "Enables custom command 3");

  gtk_grid_attach(GTK_GRID(gridCmd3), lblCmd3Enabled, 1, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(gridCmd3), settings->cmd3Enabled, 2, 2, 1, 1);

  // - label
  GtkWidget * lblCmd3Label = gtk_label_new("Label");
  gtk_widget_set_halign(lblCmd3Label, GTK_ALIGN_END);
  settings->cmd3Label = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->cmd3Label), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->cmd3Label), con->customCmd3Label->str);
  svSetTooltip(settings->cmd3Label, "The displayed label for custom command 3");

  gtk_grid_attach(GTK_GRID(gridCmd3), lblCmd3Label, 1, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(gridCmd3), settings->cmd3Label, 2, 3, 1, 1);

  // - command
  GtkWidget * lblCmd3 = gtk_label_new("Command");
  gtk_widget_set_halign(lblCmd3, GTK_ALIGN_END);
  settings->cmd3 = gtk_entry_new();
  gtk_entry_set_width_chars(GTK_ENTRY(settings->cmd3), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->cmd3), con->customCmd3->str);
  svSetTooltip(settings->cmd3, "The actual command run");

  gtk_grid_attach(GTK_GRID(gridCmd3), lblCmd3, 1, 4, 1, 1);
  gtk_grid_attach(GTK_GRID(gridCmd3), settings->cmd3, 2, 4, 1, 1);

  //
  gtk_notebook_append_page(GTK_NOTEBOOK(editTab), customCmdsPage, lblCmdsTab);

  // -- end of tab control --
  gtk_container_add(GTK_CONTAINER(boxEditParent), editTab);

  // ------------------- save / cancel buttons -------------------
  GtkWidget * boxButtons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  // save
  GtkWidget * btnSave = gtk_button_new_with_label("Save");
  gtk_widget_set_size_request(btnSave, 110, -1);
  svSetTooltip(btnSave, "Click to save these settings");
  g_signal_connect(btnSave, "clicked", G_CALLBACK(svHandleConnectionSettingsSave), settings);

  gtk_box_pack_end(GTK_BOX(boxButtons), btnSave, false, false, 3);

  // cancel
  GtkWidget * btnCancel = gtk_button_new_with_label("Cancel");
  gtk_widget_set_size_request(btnCancel, 110, -1);
  svSetTooltip(btnCancel, "Click to abandon any changes.  Caution: Any changes will be lost!");
  g_signal_connect(btnCancel, "clicked", G_CALLBACK(svHandleConnectionSettingsCancel), settings);

  gtk_box_pack_end(GTK_BOX(boxButtons), btnCancel, false, false, 3);

  //
  gtk_container_add(GTK_CONTAINER(boxEditParent), boxButtons);

  // show everything
  gtk_widget_show_all(settings->settingsWin);

  // focus the first entry
  gtk_widget_grab_focus(settings->connectionName);
}


/* handle app->parent divider position change */
void svHandlePaneDividerChange (GtkPaned * self, GtkScrollType * scroll_type, gpointer userData)
{
  if (self == NULL)
    return;

  // store handle position (basically 'list width')
  app->serverListWidth = gtk_paned_get_position(self);
}


/* handle main window state changes */
void svHandleMainWinChange (GtkWidget * self, GdkEventWindowState * event, gpointer userData)
{
  if (self == NULL || self != app->mainWin)
    return;

  // set app->maximized value based on window state
  if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
    app->maximized = true;
  else
    app->maximized = false;
}


/* set or unset menu item tooltips */
/* (connection settings and app options tooltips are set elsewhere) */
void svSetMenuItemTooltips ()
{
  // set tooltips
  svSetTooltip(app->toolsItems->requestUpdate, "Asks the remote host to send an updated screen");
  svSetTooltip(app->toolsItems->sendEnteredKeys, "Sends entered or pasted keys to the remote host");
  svSetTooltip(app->toolsItems->sendCAD, "Sends the Control+Alt+Delete key combination to the remote host");
  svSetTooltip(app->toolsItems->sendCSE, "Sends the Control+Shift+Esc key combination to the remote host");
  svSetTooltip(app->toolsItems->addNew, "Adds a new connection to the connection list");
  svSetTooltip(app->toolsItems->fullscreen, "Toggles app fullscreen mode");
  svSetTooltip(app->toolsItems->listenMode, "Toggles reverse VNC (listening) mode");
  svSetTooltip(app->toolsItems->screenshot, "Takes a screenshot of the remote host");
  svSetTooltip(app->toolsItems->appOptions, "Displays the App Options window");
  svSetTooltip(app->toolsItems->quit, "Closes all connections and quits the app");
}


/* check for, prompt and display new new connection add */
void svCheckForNewConnectionAdd ()
{
  // if nothing was added, prompt user to create a new connection
  if (app->addNewConnection == false)
    return;

  // create and edit new connection
  Connection * con = (Connection *)malloc(sizeof(Connection));

  if (con == NULL)
    return;

  svInitConnObject(con);

  // set up new connection
  g_string_assign(con->name, "(new connection)");
  g_string_assign(con->group, "General");

  // show connection editor window
  svShowConnectionEditWindow(con);
}


/* create the main app GUI */
void svCreateGUI (GtkApplication * gtkApp)
{
  // create main window
  app->mainWin = gtk_application_window_new(gtkApp);
  //gtk_application_add_window(app->gApp, GTK_WINDOW(app->mainWin));
  gtk_window_set_title(GTK_WINDOW(app->mainWin), "SpiritVNC");
  gtk_window_set_default_size(GTK_WINDOW(app->mainWin), 1024, 720);
  gtk_widget_set_hexpand(app->mainWin, false);
  gtk_widget_set_vexpand(app->mainWin, false);
  g_signal_connect(app->mainWin, "delete-event", G_CALLBACK(svDoQuit), NULL);

  // window icon
  GdkPixbuf * appIcon = gdk_pixbuf_new_from_xpm_data((const char **)xpmSpiritvnc);
  if (appIcon != NULL)
    gtk_window_set_default_icon(appIcon);

  gtk_window_set_application(GTK_WINDOW(app->mainWin), GTK_APPLICATION(gtkApp));

  // parent paned to hold server list and viewer
  app->parent = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_container_add(GTK_CONTAINER(app->mainWin), app->parent);

  // ################ TOOLS MENU - START #######################

  // tools menubar
  app->menuBar = gtk_menu_bar_new();

  // first top-level menu item
  GtkWidget * tools = gtk_menu_item_new_with_label("_Tools");
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(tools), true);
  gtk_widget_set_sensitive(GTK_WIDGET(tools), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(app->menuBar), tools);

  // submenu for tools
  GtkWidget * submenu = gtk_menu_new();

  // request update
  GtkWidget * upd = gtk_menu_item_new_with_label("Request _update");
  if (app->toolsItems != NULL)
    app->toolsItems->requestUpdate = upd;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(upd), true);
  gtk_widget_set_sensitive(GTK_WIDGET(upd), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), upd);
  g_signal_connect(upd, "activate", G_CALLBACK(svHandleRequestUpdateMenuItem), NULL);

  // ************ begin 'send' submenu *****************************

  // send submenu
  GtkWidget * ssm = gtk_menu_item_new_with_label("Send...");
  if (app->toolsItems != NULL)
    app->toolsItems->send = ssm;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(ssm), true);
  gtk_widget_set_sensitive(GTK_WIDGET(ssm), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), ssm);

  // create submenu
  GtkWidget * sendMen = gtk_menu_new();

  // attach submenu to 'send' item
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(ssm), sendMen);

  // send entered keystrokes
  GtkWidget * sks = gtk_menu_item_new_with_label("_Keys...");
  if (app->toolsItems != NULL)
    app->toolsItems->sendEnteredKeys = sks;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(sks), true);
  gtk_widget_set_sensitive(GTK_WIDGET(sks), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(sendMen), sks);
  g_signal_connect(sks, "activate", G_CALLBACK(svHandleSendEnteredKeystrokesMenuItem), NULL);

  // send ctrl+alt+del
  GtkWidget * cad = gtk_menu_item_new_with_label("_Ctrl+Alt+Del");
  if (app->toolsItems != NULL)
    app->toolsItems->sendCAD = cad;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cad), true);
  gtk_widget_set_sensitive(GTK_WIDGET(cad), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(sendMen), cad);
  g_signal_connect(cad, "activate", G_CALLBACK(svHandleSendCADMenuItem), NULL);

  // send ctrl+shift+esc
  GtkWidget * cse = gtk_menu_item_new_with_label("_Ctrl+Shift+Esc");
  if (app->toolsItems != NULL)
    app->toolsItems->sendCSE = cse;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cse), true);
  gtk_widget_set_sensitive(GTK_WIDGET(cse), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(sendMen), cse);
  g_signal_connect(cse, "activate", G_CALLBACK(svHandleSendCSEMenuItem), NULL);

  // ********* end of 'send' submenu ****************

  // add new
  GtkWidget * add = gtk_menu_item_new_with_label("Add _new connection...");
  if (app->toolsItems != NULL)
    app->toolsItems->addNew = add;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(add), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), add);
  g_signal_connect(add, "activate", G_CALLBACK(svHandleAddNewConnectionMenuItem), NULL);

  // fullscreen
  GtkWidget * ful = gtk_menu_item_new_with_label("Toggle _fullscreen");
  if (app->toolsItems != NULL)
    app->toolsItems->fullscreen = ful;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(ful), true);
  gtk_widget_set_sensitive(GTK_WIDGET(ful), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), ful);
  g_signal_connect(ful, "activate", G_CALLBACK(svDoQuit), NULL);

  // listen mode
  GtkWidget * lis = gtk_menu_item_new_with_label("Toggle _listen mode");
  if (app->toolsItems != NULL)
    app->toolsItems->listenMode = lis;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(lis), true);
  gtk_widget_set_sensitive(GTK_WIDGET(lis), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), lis);
  g_signal_connect(lis, "activate", G_CALLBACK(svDoQuit), NULL);

  // screenshot
  GtkWidget * scr = gtk_menu_item_new_with_label("_Screenshot");
  if (app->toolsItems != NULL)
    app->toolsItems->screenshot = scr;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(scr), true);
  gtk_widget_set_sensitive(GTK_WIDGET(scr), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), scr);
  g_signal_connect(scr, "activate", G_CALLBACK(svHandleScreenshotMenuItem), NULL);

  // app options
  GtkWidget * ao = gtk_menu_item_new_with_label("_App options");
  if (app->toolsItems != NULL)
    app->toolsItems->appOptions = ao;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(ao), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), ao);
  g_signal_connect(ao, "activate", G_CALLBACK(svShowAppOptionsWindow), NULL);

  // quit
  GtkWidget * qui = gtk_menu_item_new_with_label("_Quit");
  if (app->toolsItems != NULL)
    app->toolsItems->quit = qui;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(qui), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), qui);
  g_signal_connect(qui, "activate", G_CALLBACK(svDoQuit), NULL);

  // add submenu to tools
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(tools), submenu);

  //gtk_application_set_menubar(GTK_APPLICATION(gtkApp), G_MENU_MODEL(app->menuBar));

  // ################ TOOLS MENU - END #######################

  // ################# PANES - START #################

  // server list -- nearly the heart of the program
  // * (keep this here, needed for config read) *
  app->serverList = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->serverList), GTK_SELECTION_BROWSE);
  g_signal_connect(app->serverList, "button-press-event", G_CALLBACK(svHandleConnectionListClicks), NULL);

  // read in the config file and fill the connection listbox + glist
  svConfigRead();

  // set the left pane width
  gtk_paned_set_position(GTK_PANED(app->parent), app->serverListWidth);

  gtk_list_box_unselect_all(GTK_LIST_BOX(app->serverList));

  // **** left pane ****
  // parent listbox and display stack
  // menu and hostlist
  GtkWidget * leftBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
  gtk_container_set_border_width(GTK_CONTAINER(leftBox), 2);
  // add the leftBox parent container to left pane
  gtk_paned_pack1(GTK_PANED(app->parent), leftBox, false, true);

  // pack menu-bar and server list into leftBox
  gtk_box_pack_start(GTK_BOX(leftBox), app->menuBar, false, false, 0);

  // scrollable parent for server list
  app->serverListScroller = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_vexpand(app->serverListScroller, true);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(app->serverListScroller),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  // add server list to scroller
  gtk_container_add(GTK_CONTAINER(app->serverListScroller), app->serverList);
  // add server list scroller to leftBox
  gtk_box_pack_start(GTK_BOX(leftBox), app->serverListScroller, true, true, 0);

  // quicknote item label
  app->quickNoteLabel = gtk_label_new("-");
  gtk_widget_set_halign(app->quickNoteLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_start(app->quickNoteLabel, 3);
  gtk_widget_set_margin_top(app->quickNoteLabel, 7);
  // pack quicknote label in leftBox
  gtk_box_pack_start(GTK_BOX(leftBox), app->quickNoteLabel, false, false, 0);

  // quicknote last connected
  app->quickNoteLastConnected = gtk_label_new("Last connected: -");
  gtk_label_set_line_wrap(GTK_LABEL(app->quickNoteLastConnected), true);
  gtk_widget_set_halign(app->quickNoteLastConnected, GTK_ALIGN_START);
  gtk_widget_set_margin_start(app->quickNoteLastConnected, 3);
  gtk_widget_set_margin_top(app->quickNoteLastConnected, 7);
  // pack quicknote label in leftBox
  gtk_box_pack_start(GTK_BOX(leftBox), app->quickNoteLastConnected, false, false, 0);

  // connection last error scroller
  GtkWidget * errorScroller = gtk_scrolled_window_new(NULL, NULL);
  // add quicknote scroller to leftBox
  gtk_box_pack_start(GTK_BOX(leftBox), errorScroller, false, false, 0);

  // connection last error
  app->quickNoteLastError = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(app->quickNoteLastError), false);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(app->quickNoteLastError), 5);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(app->quickNoteLastError), 2);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(app->quickNoteLastError), 5);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(app->quickNoteLastError), 2);
  // pack quicknote label in leftBox
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->quickNoteLastError), GTK_WRAP_WORD_CHAR);
  gtk_container_add(GTK_CONTAINER(errorScroller), app->quickNoteLastError);

  // quicknote scroller
  GtkWidget * quickScroller = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_size_request(quickScroller, -1, 180); //160);
  // add quicknote scroller to leftBox
  gtk_box_pack_start(GTK_BOX(leftBox), quickScroller, false, false, 0);

  // quick note editor
  app->quickNoteView = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(app->quickNoteView), false);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(app->quickNoteView), 5);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(app->quickNoteView), 2);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(app->quickNoteView), 5);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(app->quickNoteView), 2);
  // add quickNoteView to quicknote scroller
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->quickNoteView), GTK_WRAP_WORD_CHAR);
  gtk_container_add(GTK_CONTAINER(quickScroller), app->quickNoteView);

  app->quickNoteLastErrorBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->quickNoteLastError));
  app->quickNoteBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->quickNoteView));

  // **** right pane ****
  // vnc viewer
  // display stack for vnc display objects
  // scrollable parent for stack
  app->displayStackScroller = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(app->displayStackScroller),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_paned_pack2(GTK_PANED(app->parent), app->displayStackScroller, true, true);

  //// display stack for vnc display objects
  app->displayStack = gtk_stack_new();
  gtk_stack_set_homogeneous(GTK_STACK(app->displayStack), false);
  gtk_stack_set_interpolate_size(GTK_STACK(app->displayStack), false);
  gtk_widget_set_hexpand(app->displayStack, false);
  gtk_widget_set_vexpand(app->displayStack, false);
  gtk_container_add(GTK_CONTAINER(app->displayStackScroller), app->displayStack);

  // ################# PANES - END ################

  // set or unset tooltips
  svSetMenuItemTooltips();

  // show everything
  gtk_widget_show_all(app->mainWin);

  // if saved main window state was maximized, maximize the main window
  if (app->maximized == true)
    gtk_window_maximize(GTK_WINDOW(app->mainWin));

  // size the hostlist pane
  gtk_paned_set_position(GTK_PANED(app->parent), app->serverListWidth);

  // connect the pane handle handler handlething handlez (leave this here, not by widget creation)
  g_signal_connect(app->parent, "notify::position", G_CALLBACK(svHandlePaneDividerChange), NULL);

  // handle window max / un-max
  g_signal_connect(app->mainWin, "window-state-event", G_CALLBACK(svHandleMainWinChange), NULL);

  // check if we need to show the 'add new connection' prompt
  svCheckForNewConnectionAdd();
}


/* create new config folder(s) and file if none exists */
bool svConfigCreateNew (bool createEmptyConfigFile)
{
  // create app's config folder, making parent folders as necessary
  int result = g_mkdir_with_parents(app->appConfigDir->str, 0755);

  // error in folder creation
  if (result == -1)
  {
    svLog("svConfigCreateNew - Can't create config folder(s)", false);
    return false;
  }

  if (createEmptyConfigFile == true)
  {
    // only create a new empty file if one doesn't exist already
    if (g_file_test(app->appConfigFile->str, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS) == false)
    {
      svLog("svConfigCreateNew - Creating new empty config file", false);

      // --- attempt to write empty config file ---
      if (g_file_set_contents(app->appConfigFile->str, "\n", -1, NULL) == false)
      {
        svLog("svConfigCreateNew - Error: SpiritVNC could not write empty config file", false);
        return false;
      }
    }
  }

  return true;
}


/* insert a row into the connection listbox */
void svInsertHostListRow (char * rowText, int idx)
{
  if (rowText == NULL)
  {
    svLog("svInsertHostlistRow - rowText is null", false);
    return;
  }

  // box
  GtkWidget * box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);

  // image
  GdkPixbuf * pb;

  if (strcmp(rowText, "") != 0)
    pb = gdk_pixbuf_new_from_xpm_data((const char **)xpmDisconnected);
  else
    pb = gdk_pixbuf_new_from_xpm_data((const char **)xpmBlank);

  GtkWidget * img = gtk_image_new_from_pixbuf(pb);

  if (img == NULL)
    svLog("svInsertHostListRow - img is null", false);

  // label
  GtkWidget * label = gtk_label_new(rowText);
  gtk_widget_set_halign(label, GTK_ALIGN_START);

  // add img and label to box
  gtk_box_pack_start(GTK_BOX(box), img, false, false, 3);
  gtk_box_pack_start(GTK_BOX(box), label, false, false, 0);

  gtk_widget_set_visible(img, true);
  gtk_widget_set_visible(label, true);
  gtk_widget_set_visible(box, true);

  // add box to connections listbox
  gtk_list_box_insert(GTK_LIST_BOX(app->serverList), box, idx);
}


/* read the config file and fill the connections listbox + glist */
void svConfigRead ()
{
  GError * readError = NULL;
  gsize fileSize = 0;  // leave this as gsize

  char * strIn = NULL;

  // check and create any missing config folders
  if (svConfigCreateNew(true) == false)
  {
    svLog("svConfigRead - Could not create new config dirs or file - quitting app", false);
    exit(-1);
  }

  // open config file for reading
  if (g_file_get_contents(app->appConfigFile->str, &strIn, &fileSize, &readError) == false)
  {
    char errStr[FILENAME_MAX] = {0};

    if (readError != NULL)
      snprintf(errStr, FILENAME_MAX - 1, "svConfigRead - Error opening config file\nError: %s", readError->message);
    else
      strcpy(errStr, "svConfigRead - Error opening config file - No error reported");

    svLog(errStr, false);
  }

  // log if strIn is null
  if (strIn == NULL)
    svLog("svConfigRead - File read variable is NULL after read attempt", false);

  // config file read-in variables
  char strLine[FILENAME_MAX];
  uint32_t intPos = 0;
  char * charEqPos = NULL;
  int eqPos = -1;
  char charIn = 0;

  char strProp[100] = {0};
  char strVal[FILENAME_MAX] = {0};

  char strLastGroup[FILENAME_MAX] = {0};

  Connection * con = (Connection *)malloc(sizeof(Connection));
  svInitConnObject(con);

  // go through file content string and parse properties / values
  for (unsigned int ii = 0; ii < fileSize; ii++)
  {
    charIn = strIn[ii];

    // newline, process line
    if (charIn == '\n' || charIn == '\r')
    {
      strLine[intPos] = '\0';
      intPos = 0;

      // if there's = sign in the line, process it
      charEqPos = strchr(strLine, '=');

      if (charEqPos != NULL)
      {
        // zero out prop and val variables
        memset(strProp, '\0', 100);
        memset(strVal, '\0', FILENAME_MAX);

        // find position of first '=' char
        eqPos = charEqPos - strLine;

        // copy prop and val to appropriate variables
        strncpy(strProp, strLine, eqPos);
        strncpy(strVal, charEqPos + 1, FILENAME_MAX - 1);

        //printf("prop: %s, val: %s\n", strProp, strVal);

        // if we have an empty / null prop, don't process this line
        if (strProp[0] == '\0')
          continue;

        // fix null values
        if (strVal[0] == '\0')
          strcpy(strVal, "");

        // ** start assigning values to properties **

        // ===== app settings =====

        // * serverListWidth *
        if (strcmp(strProp, "hostlistwidth") == 0)
        {
          app->serverListWidth = atoi(strVal);

          if (app->serverListWidth < 1)
            app->serverListWidth = 120;
        }

        // * showTooltips *
        if (strcmp(strProp, "showtooltips") == 0)
          app->showTooltips = svStringToBool(strVal);

        // * maximized *
        if (strcmp(strProp, "maximized") == 0)
          app->maximized = svStringToBool(strVal);

        // * log to file *
        if (strcmp(strProp, "logtofile") == 0)
          app->logToFile = svStringToBool(strVal);

        // * debug mode *
        if (strcmp(strProp, "debugmode") == 0)
          app->debugMode = svStringToBool(strVal);

        // * scan wait time *
        if (strcmp(strProp, "scantimeout") == 0)
          app->scanTimeout = atoi(strVal);

        // * vnc connect timeout *
        if (strcmp(strProp, "vnctimeout") == 0)
          app->vncConnectWaitTime = atoi(strVal);

        // * ssh command *
        if (strcmp(strProp, "sshcommand") == 0)
          g_string_assign(app->sshCommand, strVal);

        // * ssh connect timeout *
        if (strcmp(strProp, "sshtimeout") == 0)
          app->sshConnectWaitTime = atoi(strVal);

        // ===== individual connection settings =====

        // * connName *
        if (strcmp(strProp, "host") == 0)
        {
          // if there was a previous con object, add it
          // to the connections GList
          if (con->name != NULL && strcmp(con->name->str, "") != 0)
          {
            // append connection to connection list
            app->connections = g_list_append(app->connections, con);

            // create box, image and label for listbox row
            svInsertHostListRow(con->name->str, -1);

            app->addNewConnection = false;
          }

          con = (Connection *)malloc(sizeof(Connection));
          svInitConnObject(con);

          // connection name
          g_string_assign(con->name, strVal);
        }

        // * connGroup *
        if (strcmp(strProp, "group") == 0)
        {
          // add a separator if it's a new group
          if (strcmp(strVal, strLastGroup) != 0)
          {
            app->connections = g_list_append(app->connections, NULL);

            svInsertHostListRow("", -1);
          }

          g_string_assign(con->group, strVal);
          strncpy(strLastGroup, strVal, FILENAME_MAX);
        }

        // * address *
        if (strcmp(strProp, "address") == 0 || strcmp(strProp, "hostaddress") == 0)
          g_string_assign(con->address, strVal);

        // * type *
        if (strcmp(strProp, "type") == 0)
        {
          if (strcmp(strVal, "0") == 0 || strcmp(strVal, "v") == 0)
            con->type = SV_TYPE_VNC;
          else if (strcmp(strVal, "1") == 0 || strcmp(strVal, "s") == 0)
            con->type = SV_TYPE_VNC_OVER_SSH;
          else
            con->type = SV_TYPE_VNC;
        }

        // * vncPort *
        if (strcmp(strProp, "vncport") == 0)
          g_string_assign(con->vncPort, strVal);

        // * vncPass *
        if (strcmp(strProp, "vncpass") == 0)
          g_string_assign(con->vncPass, strVal);

        // * vncLoginUser *
        if (strcmp(strProp, "vncloginuser") == 0)
          g_string_assign(con->vncLoginUser, strVal);

        // * vncLoginPass *
        if (strcmp(strProp, "vncloginpass") == 0)
          g_string_assign(con->vncLoginPass, strVal);

        // * scaling *
        if (strcmp(strProp, "scale") == 0)
          con->scale = svStringToBool(strVal);

        // * showRemoteCursor *
        if (strcmp(strProp, "showremotecursor") == 0)
          con->showRemoteCursor = svStringToBool(strVal);

        // * imageQuality *
        if (strcmp(strProp, "quality") == 0)
        {
          if (strcmp(strVal, "0") == 0)
            con->quality = SV_QUAL_LOW;
          else if (strcmp(strVal, "1") == 0 || strcmp(strVal, "5") == 0)
            con->quality = SV_QUAL_MEDIUM;
          else if (strcmp(strVal, "2") == 0 || strcmp(strVal, "9") == 0)
            con->quality = SV_QUAL_FULL;
          else
            con->quality = SV_QUAL_DEFAULT;
        }

        // * lossy encoding *
        if (strcmp(strProp, "lossyencoding") == 0)
          con->lossyEncoding = svStringToBool(strVal);

        // * sshPort *
        if (strcmp(strProp, "sshport") == 0)
          g_string_assign(con->sshPort, strVal);

        // * sshPrivKeyfile *
        if (strcmp(strProp, "sshkeyprivate") == 0)
          g_string_assign(con->sshPrivKeyfile, strVal);

        // * sshUser *
        if (strcmp(strProp, "sshuser") == 0)
          g_string_assign(con->sshUser, strVal);

        // * f12Macro *
        if (strcmp(strProp, "f12macro") == 0)
          g_string_assign(con->f12Macro, strVal);

        // * quicknote *
        if (strcmp(strProp, "quicknote") == 0)
          g_string_assign(con->quickNote, strVal);

        // * custom command 1 enabled *
        if (strcmp(strProp, "customcmd1enabled") == 0 || strcmp(strProp, "customcommand1enabled") == 0)
          con->customCmd1Enabled = svStringToBool(strVal);

        // * custom command 1 label *
        if (strcmp(strProp, "customcmd1label") == 0 || strcmp(strProp, "customcommand1label") == 0)
          g_string_assign(con->customCmd1Label, strVal);

        // * custom command 1 *
        if (strcmp(strProp, "customcmd1") == 0 || strcmp(strProp, "customcommand1") == 0)
          g_string_assign(con->customCmd1, strVal);

        // * custom command 2 enabled *
        if (strcmp(strProp, "customcmd2enabled") == 0 || strcmp(strProp, "customcommand2enabled") == 0)
          con->customCmd2Enabled = svStringToBool(strVal);

        // * custom command 2 label *
        if (strcmp(strProp, "customcmd2label") == 0 || strcmp(strProp, "customcommand2label") == 0)
          g_string_assign(con->customCmd2Label, strVal);

        // * custom command 2 *
        if (strcmp(strProp, "customcmd2") == 0 || strcmp(strProp, "customcommand2") == 0)
          g_string_assign(con->customCmd2, strVal);

        // * custom command 3 enabled *
        if (strcmp(strProp, "customcmd3enabled") == 0 || strcmp(strProp, "customcommand3enabled") == 0)
          con->customCmd3Enabled = svStringToBool(strVal);

        // * custom command 3 label *
        if (strcmp(strProp, "customcmd3label") == 0 || strcmp(strProp, "customcommand3label") == 0)
          g_string_assign(con->customCmd3Label, strVal);

        // * custom command 3 *
        if (strcmp(strProp, "customcmd3") == 0 || strcmp(strProp, "customcommand3") == 0)
          g_string_assign(con->customCmd3, strVal);

        // * last connect time *
        if (strcmp(strProp, "lastconnecttime") == 0)
          g_string_assign(con->lastConnectTime, strVal);
      }
    }
    else
    {
      strLine[intPos] = charIn;
      intPos ++;
    }
  }

  // add last con item, if not null
  if (con != NULL && strcmp(con->name->str, "") != 0)
  {
    // add connection to app->connections list
    app->connections = g_list_append(app->connections, con);

    svInsertHostListRow(con->name->str, -1);

    app->addNewConnection = false;
  }

  // free strIn
  g_free(strIn);

  // set or unset hostlist item tooltips
  svSetHostlistItemsTooltips();
}


/* write the configuration file */
void svConfigWrite ()
{
  if (svConfigCreateNew(false) == false)
  {
    svLog("svConfigWrite - Could not create new config dirs or file", false);
    return;
  }

  GString * outStr = g_string_new(NULL);

  // header
  g_string_append(outStr, "# SpiritVNC - GTK 3 version\n");
  g_string_append(outStr, "# 2022-2025 Will Brokenbourgh - to God be the glory!\n#\n");

  // config version
  g_string_append(outStr, "configver=1.0\n");

  // --- app options ---

  // list width
  g_string_append_printf(outStr, "hostlistwidth=%i\n", app->serverListWidth);

  // show tooltips
  g_string_append_printf(outStr, "showtooltips=%i\n", svIntFromBool(app->showTooltips));

  // log to file
  g_string_append_printf(outStr, "logtofile=%i\n", svIntFromBool(app->logToFile));

  // debug mode
  g_string_append_printf(outStr, "debugmode=%i\n", svIntFromBool(app->debugMode));

  // window maximized
  g_string_append_printf(outStr, "maximized=%i\n", svIntFromBool(app->maximized));

  // scan wait time
  g_string_append_printf(outStr, "scantimeout=%i\n", app->scanTimeout);

  // vnc connect timeout
  g_string_append_printf(outStr, "vnctimeout=%i\n", app->vncConnectWaitTime);

  // ssh command
  g_string_append_printf(outStr, "sshcommand=%s\n", app->sshCommand->str);

  // ssh connect timeout
  g_string_append_printf(outStr, "sshtimeout=%i\n", app->sshConnectWaitTime);

  // space
  g_string_append(outStr, "\n");

  // --- per-connection settings ---
  Connection * con = NULL;

  // go through connection glist and add each connection's settings
  for (GList * l = app->connections; l != NULL; l = l->next)
  {
    con = (Connection *)l->data;

    if (con == NULL || strcmp(con->name->str, "") == 0)
      continue;

    g_string_append_printf(outStr, "host=%s\n", con->name->str);
    g_string_append_printf(outStr, "group=%s\n", con->group->str);
    g_string_append_printf(outStr, "address=%s\n", con->address->str);
    g_string_append_printf(outStr, "type=%i\n", con->type);
    g_string_append_printf(outStr, "vncport=%s\n", con->vncPort->str);
    g_string_append_printf(outStr, "vncpass=%s\n", con->vncPass->str);
    g_string_append_printf(outStr, "vncloginuser=%s\n", con->vncLoginUser->str);
    g_string_append_printf(outStr, "vncloginpass=%s\n", con->vncLoginPass->str);
    g_string_append_printf(outStr, "scale=%i\n", svIntFromBool(con->scale));
    g_string_append_printf(outStr, "lossyencoding=%i\n", svIntFromBool(con->lossyEncoding));
    g_string_append_printf(outStr, "quality=%i\n", con->quality);
    g_string_append_printf(outStr, "sshuser=%s\n", con->sshUser->str);
    g_string_append_printf(outStr, "sshpass=%s\n", con->sshPass->str);
    g_string_append_printf(outStr, "sshport=%s\n", con->sshPort->str);
    g_string_append_printf(outStr, "sshkeyprivate=%s\n", con->sshPrivKeyfile->str);
    g_string_append_printf(outStr, "f12macro=%s\n", con->f12Macro->str);
    g_string_append_printf(outStr, "quicknote=%s\n", con->quickNote->str);
    g_string_append_printf(outStr, "lastconnecttime=%s\n", con->lastConnectTime->str);
    g_string_append_printf(outStr, "customcmd1enabled=%i\n", svIntFromBool(con->customCmd1Enabled));
    g_string_append_printf(outStr, "customcmd1label=%s\n", con->customCmd1Label->str);
    g_string_append_printf(outStr, "customcmd1=%s\n", con->customCmd1->str);
    g_string_append_printf(outStr, "customcmd2enabled=%i\n", svIntFromBool(con->customCmd2Enabled));
    g_string_append_printf(outStr, "customcmd2label=%s\n", con->customCmd2Label->str);
    g_string_append_printf(outStr, "customcmd2=%s\n", con->customCmd2->str);
    g_string_append_printf(outStr, "customcmd3enabled=%i\n", svIntFromBool(con->customCmd3Enabled));
    g_string_append_printf(outStr, "customcmd3label=%s\n", con->customCmd3Label->str);
    g_string_append_printf(outStr, "customcmd3=%s\n", con->customCmd3->str);

    // empty line after this connection
    g_string_append(outStr, "\n");
  }

  // empty line at end of file
  g_string_append(outStr, "\n");

  // --- attempt to write config file out ---
  if (g_file_set_contents(app->appConfigFile->str, outStr->str, -1, NULL) == false)
    svLog("svConfigWrite - Error: SpiritVNC could not write config file", false);
}


/* return a connection object based on connection name */
Connection * svConnectionFromName (const char * text)
{
  Connection * con = NULL;

  // go through connections glist and return correct connection
  for (GList * l = app->connections; l != NULL; l = l->next)
  {
    con = (Connection *)l->data;

    // if connection isn't null and matches the connection
    // name, return con object
    if (con != NULL && strcmp(text, con->name->str) == 0)
      return con;
  }

  return NULL;
}


/* sets a connection's icon in the connection list */
void svSetIconFromConnectionName (const char * text, unsigned int state)
{
  GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

  // loop through each row
  for (GList * l = rows; l != NULL; l = l->next)
  {
    GtkWidget * row = GTK_WIDGET(l->data);

    // get box that holds status image and label
    GtkWidget * childBox = gtk_bin_get_child(GTK_BIN(row));

    if (childBox == NULL)
      continue;

    // get box's children
    GList * boxChildren = gtk_container_get_children(GTK_CONTAINER(childBox));

    GtkWidget * img = g_list_nth_data(boxChildren, 0);
    GtkWidget * label = g_list_nth_data(boxChildren, 1);

    // get label's text, if any
    char * lText = (char *)gtk_label_get_text(GTK_LABEL(label));

    if (lText == NULL)
      continue;

    // if the label text matches, modify the icon
    if (strcmp(lText, text) == 0)
    {
      if (img != NULL)
      {
        gtk_image_clear(GTK_IMAGE(img));

        // image
        GdkPixbuf * pb;

        switch (state)
        {
          case SV_STATE_DISCONNECTED:
            pb = gdk_pixbuf_new_from_xpm_data((const char **)xpmDisconnected);
            break;

          case SV_STATE_CONNECTED:
            pb = gdk_pixbuf_new_from_xpm_data((const char **)xpmConnected);
            break;

          case SV_STATE_WAITING:
            pb = gdk_pixbuf_new_from_xpm_data((const char **)xpmWait);
            break;

          case SV_STATE_TIMEOUT:
            pb = gdk_pixbuf_new_from_xpm_data((const char **)xpmTimeout);
            break;

          case SV_STATE_ERROR:
            pb = gdk_pixbuf_new_from_xpm_data((const char **)xpmError);
            break;

          default:
            pb = gdk_pixbuf_new_from_xpm_data((const char **)xpmBlank);
        }

        // set the icon
        gtk_image_set_from_pixbuf(GTK_IMAGE(img), pb);
      }
    }
  }
}


/* closes all connections (typically when exiting program) */
void svEndAllConnections ()
{
  // end all viewers first
  Connection * con = NULL;

  // go through connection glist and return correct connection
  for (GList * l = app->connections; l != NULL; l = l->next)
  {
    con = (Connection *)l->data;

    if (con != NULL && con->state == SV_STATE_CONNECTED && con->vncObj != NULL)
      vnc_display_close(VNC_DISPLAY(con->vncObj));
  }
}


/* shut everything down and quit the app */
void svDoQuit ()
{
  svEndAllConnections();

  svConfigWrite();

  svLog("--- App ending ---", false);

  g_application_quit(G_APPLICATION(app->gApp));
}


/*  find unused TCP port in 'ephemeral / temporary' range  */
int svFindFreeTcpPort ()
{
  int nSock = 0;
  struct sockaddr_in structSockAddress;

  structSockAddress.sin_family = AF_INET;
  structSockAddress.sin_addr.s_addr = htonl(INADDR_ANY);

  nSock = socket(AF_INET, SOCK_STREAM, 0);

  if (nSock < 0)
  {
    svLog("svFindFreeTcpPort - ERROR - Cannot create socket for svFindFreeTCPPort", false);
    return 0;
  }

  // go through a range of startingLocalPort to + 99 to see if we can use for ssh forwarding
  for (uint16_t nPort = 49152; nPort < 65534; nPort ++)
  {
    structSockAddress.sin_port = htons((unsigned short)nPort);

    // don't clobber the listening port
    if (nPort == 5500)
      continue;

    // if nothing is on this port and it's not the reverse vnc port, return the port number
    if (bind(nSock, (const struct sockaddr *)&structSockAddress, sizeof(structSockAddress)) == 0)
    {
      close(nSock);
      return nPort;
    }
  }

  close(nSock);

  return 0;
}


/* deal with server list clicks */
gboolean svHandleConnectionListClicks (GtkWidget * widget, GdkEvent * event, void * data)
{
  GdkEventType type = gdk_event_get_event_type(event);

  // if not a left or right button press, get out
  if (type != GDK_BUTTON_PRESS && type != GDK_2BUTTON_PRESS)
    return false;

  unsigned int clickCount = 0;
  unsigned int button = 0;

  // set last button number
  if (gdk_event_get_button(event, &button) == false)
    return false;

  // set last click count
  if (gdk_event_get_click_count(event, &clickCount) == false)
    return false;

  double x = 0;
  double y = 0;

  // get the row's internal name from the mouse's y position
  if (gdk_event_get_coords(event, &x, &y) == false)
    return false;

  // hide any active display if list is single-clicked
  if (button != GDK_BUTTON_SECONDARY && clickCount == 1)
  {
    GtkWidget * currentDisplay = gtk_stack_get_visible_child(GTK_STACK(app->displayStack));

    if (currentDisplay != NULL)
      gtk_widget_hide(currentDisplay);
  }

  // get the clicked row
  GtkListBoxRow * row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(widget), y);

  if (row == NULL)
    return false;

  // get box that holds status image and label
  GtkWidget * childBox = gtk_bin_get_child(GTK_BIN(row));

  if (childBox == NULL)
    return false;

  // get box's children
  GList * boxChildren = gtk_container_get_children(GTK_CONTAINER(childBox));

  GtkWidget * label = g_list_nth_data(boxChildren, 1);

  if (label == NULL)
    return false;

  // get label's text, if any
  const char * lText = gtk_label_get_text(GTK_LABEL(label));

  // get the matching connection from the label's text
  Connection * con = svConnectionFromName(lText);

  // **### DON'T check con for null here!! ###**

  // do stuffs based on mouse button and number of clicks
  if (button > 0 && clickCount > 0)
  {
    // single left-click
    if (button == GDK_BUTTON_PRIMARY && clickCount == 1)
      svConnectionSwitch(con);

    // double left-click
    if (button == GDK_BUTTON_PRIMARY && clickCount == 2)
      svConnectionCreate(con);

    // single right-click
    if (button == GDK_BUTTON_SECONDARY && clickCount == 1)
      svConnectionRightClick(con);
  }

  return false;
}


/* return the int index of the currently-selected connection listbox item */
int svGetSelectedConnectionListItemIndex ()
{
  // figure out which connection is selected
  GtkListBoxRow * selectedRow = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->serverList));

  if (selectedRow == NULL)
    return -1;

  // get the connection's index
  return gtk_list_box_row_get_index(selectedRow);
}


/* return the Connection * of the currently-selected connection listbox item */
Connection * svGetSelectedConnectionListConnection ()
{
  // get the connection's index
  int idx = svGetSelectedConnectionListItemIndex();

  if (idx < 0)
    return NULL;

  // get the connections glist item for this index
  Connection * con = NULL;
  con = (Connection *)g_list_nth_data(app->connections, idx);

  return con;
}


/* menu item handler - send update request to currently-selected server */
void svHandleRequestUpdateMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  // get the connections glist item for this index
  Connection * con = svGetSelectedConnectionListConnection();

  if (con == NULL || strcmp(con->name->str, "") == 0)
    return;

  vnc_display_request_update(VNC_DISPLAY(con->vncObj));
}


/* menu item handler - send Ctrl+Alt+Del to currently-selected server */
void svHandleSendCADMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  // get the connections glist item for this index
  Connection * con = svGetSelectedConnectionListConnection();

  if (con == NULL || strcmp(con->name->str, "") == 0)
    return;

  guint keys[] = { GDK_KEY_Control_L, GDK_KEY_Alt_L, GDK_KEY_Delete };

  vnc_display_send_keys(VNC_DISPLAY(con->vncObj), keys, sizeof(keys)/sizeof(keys[0]));
}


/* menu item handler - send Ctrl+Shift+Esc to currently-selected server */
void svHandleSendCSEMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  // get the connections glist item for this index
  Connection * con = svGetSelectedConnectionListConnection();

  if (con == NULL || strcmp(con->name->str, "") == 0)
    return;

  guint keys[] = { GDK_KEY_Control_L, GDK_KEY_Shift_L, GDK_KEY_Escape };

  vnc_display_send_keys(VNC_DISPLAY(con->vncObj), keys, sizeof(keys)/sizeof(keys[0]));
}


/* menu item handler - shows 'send keys' window */
void svHandleSendEnteredKeystrokesMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  SendKeysObj * obj = malloc(sizeof(SendKeysObj));

  if (obj == NULL)
    return;

  // get the connections glist item for this index
  Connection * con = svGetSelectedConnectionListConnection();

  if (con == NULL || strcmp(con->name->str, "") == 0)
    return;

  // compose title string
  char titleStr[FILENAME_MAX] = {0};
  snprintf(titleStr, FILENAME_MAX, "Send keys to '%s' - SpiritVNC", con->name->str);

  // create output window
  GtkWidget * sendKeysWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_modal(GTK_WINDOW(sendKeysWin), true);

  // set window title
  gtk_window_set_title(GTK_WINDOW(sendKeysWin), titleStr);
  gtk_window_set_modal(GTK_WINDOW(sendKeysWin), true);
  gtk_window_set_resizable(GTK_WINDOW(sendKeysWin), false);

  // parent box
  GtkWidget * boxParent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
  gtk_container_set_border_width(GTK_CONTAINER(boxParent), 5);
  gtk_container_add(GTK_CONTAINER(sendKeysWin), boxParent);

  // container grid
  GtkWidget * grid = gtk_grid_new();
  gtk_grid_set_column_homogeneous(GTK_GRID(grid), false);
  gtk_container_set_border_width(GTK_CONTAINER(grid), 7);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 7);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 5);

  // add grid to parent box
  gtk_box_pack_start(GTK_BOX(boxParent), grid, false, false, 0);

  // out label
  GtkWidget * lblIn = gtk_label_new("Enter or paste keys / text to send below");
  gtk_widget_set_halign(lblIn, GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(grid), lblIn, 1, 1, 1, 1);

  // scroller for textview
  GtkWidget * scroll = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_size_request(scroll, 500, 400);

  gtk_grid_attach(GTK_GRID(grid), scroll, 1, 3, 1, 1);

  // output textview
  GtkWidget * textIn = gtk_text_view_new();
  //gtk_text_view_set_editable(GTK_TEXT_VIEW(textOut), false);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(textIn), true);

  // add textview to scroller
  gtk_container_add(GTK_CONTAINER(scroll), textIn);

  // ------------------- set obj fields --------------
  obj->win = sendKeysWin;
  obj->textView = textIn;
  obj->con = con;

  // ------------------- send / cancel buttons -------------------
  GtkWidget * boxButtons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  // send
  GtkWidget * btnSend = gtk_button_new_with_label("Send");
  gtk_widget_set_size_request(btnSend, 110, -1);
  svSetTooltip(btnSend, "Click to send this text");
  g_signal_connect(btnSend, "clicked", G_CALLBACK(svHandleSendEnteredKeystrokesSend), obj);

  gtk_box_pack_end(GTK_BOX(boxButtons), btnSend, false, false, 3);

  // cancel
  GtkWidget * btnCancel = gtk_button_new_with_label("Cancel");
  gtk_widget_set_size_request(btnCancel, 110, -1);
  svSetTooltip(btnCancel, "Click to close this window without sending");
  g_signal_connect(btnCancel, "clicked", G_CALLBACK(svHandleSendEnteredKeystrokesCancel), obj);

  gtk_box_pack_end(GTK_BOX(boxButtons), btnCancel, false, false, 3);
  //
  gtk_container_add(GTK_CONTAINER(boxParent), boxButtons);

  // show everything
  gtk_widget_show_all(sendKeysWin);
}


/* menu item handler - run custom command */
void svHandleCustomCommandMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  const char * cmd = (const char *)userData;

  if (cmd == NULL)
    return;

  GError * gError = NULL;
  char * stdOut = NULL;

  // run the command synchronous, will block
  if (g_spawn_command_line_sync(cmd, &stdOut, NULL, NULL, &gError) == false)
  {
    GtkWidget * dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(app->mainWin),
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_ERROR,
                                      GTK_BUTTONS_CLOSE,
                                      "<b>Error running custom command</b>\n\n%s",
                                      gError->message);
     gtk_dialog_run(GTK_DIALOG(dialog));
     gtk_widget_destroy(dialog);
  }
  else
  {
    // show command output if not null or empty
    if (stdOut != NULL && strcmp(stdOut, "") != 0)
    {
      // create output window
      GtkWidget * outWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_window_set_modal(GTK_WINDOW(outWin), true);

      // set window title
      gtk_window_set_title(GTK_WINDOW(outWin), "Custom command output - SpiritVNC");
      gtk_window_set_modal(GTK_WINDOW(outWin), true);
      gtk_window_set_resizable(GTK_WINDOW(outWin), false);

      // parent box
      GtkWidget * boxParent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
      gtk_container_set_border_width(GTK_CONTAINER(boxParent), 5);
      gtk_container_add(GTK_CONTAINER(outWin), boxParent);

      // container grid
      GtkWidget * grid = gtk_grid_new();
      gtk_grid_set_column_homogeneous(GTK_GRID(grid), false);
      gtk_container_set_border_width(GTK_CONTAINER(grid), 7);
      gtk_grid_set_column_spacing(GTK_GRID(grid), 7);
      gtk_grid_set_row_spacing(GTK_GRID(grid), 5);

      // add grid to parent box
      gtk_box_pack_start(GTK_BOX(boxParent), grid, false, false, 0);

      // out label
      GtkWidget * lblOut = gtk_label_new("Custom command had output");
      gtk_widget_set_halign(lblOut, GTK_ALIGN_START);
      gtk_grid_attach(GTK_GRID(grid), lblOut, 1, 1, 1, 1);

      // command entry (read-only, to show command)
      GtkWidget * entryCmd = gtk_entry_new();
      gtk_editable_set_editable(GTK_EDITABLE(entryCmd), false);
      gtk_widget_set_can_focus(entryCmd, false);
      gtk_entry_set_text(GTK_ENTRY(entryCmd), cmd);

      gtk_grid_attach(GTK_GRID(grid), entryCmd, 1, 2, 1, 1);

      // scroller for textview
      GtkWidget * scroll = gtk_scrolled_window_new(NULL, NULL);
      gtk_widget_set_size_request(scroll, 500, 400);

      gtk_grid_attach(GTK_GRID(grid), scroll, 1, 3, 1, 1);

      // output textview
      GtkWidget * textOut = gtk_text_view_new();
      gtk_text_view_set_editable(GTK_TEXT_VIEW(textOut), false);
      gtk_text_view_set_monospace(GTK_TEXT_VIEW(textOut), true);

      // set textview text
      GtkTextBuffer * tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textOut));
      gtk_text_buffer_set_text(tb, stdOut, -1);

      // add textview to scroller
      gtk_container_add(GTK_CONTAINER(scroll), textOut);

      // ------------------- ok button -------------------
      GtkWidget * boxButtons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

      // okay
      GtkWidget * btnOk = gtk_button_new_with_label("Ok");
      gtk_widget_set_size_request(btnOk, 110, -1);
      g_signal_connect(btnOk, "clicked", G_CALLBACK(svHandleCommandOutputOk), outWin);

      gtk_box_pack_end(GTK_BOX(boxButtons), btnOk, false, false, 3);
      //
      gtk_container_add(GTK_CONTAINER(boxParent), boxButtons);

      // show everything
      gtk_widget_show_all(outWin);
    }
  }

  if (gError != NULL)
    g_error_free(gError);
}


/* menu item handler - call connection start */
void svHandleConnectMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;

  if (con == NULL)
    return;

  svConnectionCreate(con);
}


/* menu item handler - call connection end */
void svHandleDisconnectMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;

  if (con == NULL)
    return;

  svConnectionEnd(con);
}


/* menu item handler - call connection editor */
void svHandleEditMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;

  if (con == NULL)
    return;

  svShowConnectionEditWindow(con);
}


/* menu item handler - store right-clicked connection's f12 macro (NOT on clipboard!) */
void svHandleCopyF12MacroMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;

  if (con == NULL)
    return;

  g_string_assign(app->f12Storage, con->f12Macro->str);
}


/* menu item handler - set right-clicked connection's f12 macro (NOT from clipboard!) */
void svHandlePasteF12MacroMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  //Connection * con = (Connection *)userData;

  //if (con == NULL)
    //return;
}


/* menu item handler - delete a connection */
void svHandleDeleteMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;

  if (con == NULL)
    return;

  GtkWidget * dialog = gtk_message_dialog_new(GTK_WINDOW(app->mainWin),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_QUESTION,
                                  GTK_BUTTONS_YES_NO,
                                  "Are you sure you want to delete '%s'?",
                                  con->name->str);
  int res = gtk_dialog_run(GTK_DIALOG(dialog));

  if (res == GTK_RESPONSE_YES)
  {
    Connection * conTemp = NULL;

    // go through connection glist and remove connection
    for (GList * l = app->connections; l != NULL; l = l->next)
    {
      conTemp = (Connection *)l->data;

      // found matching connection in connections glist
      if (conTemp != NULL && conTemp == con)
      {
        app->connections = g_list_remove_all(app->connections, con);
        break;
      }
    }

    // get all connection listbox rows
    GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

    // loop through each row
    for (GList * l = rows; l != NULL; l = l->next)
    {
      GtkWidget * row = GTK_WIDGET(l->data);

      // get box that holds status image and label
      GtkWidget * childBox = gtk_bin_get_child(GTK_BIN(row));

      if (childBox == NULL)
        continue;

      // get box's children
      GList * boxChildren = gtk_container_get_children(GTK_CONTAINER(childBox));

      GtkWidget * label = g_list_nth_data(boxChildren, 1);

      if (label != NULL && GTK_IS_LABEL(label))
      {
        const char * text = gtk_label_get_text(GTK_LABEL(label));

        // found matching row string
        if (strcmp(text, con->name->str) == 0)
        {
          // destroy the child widget (will remove from container too)
          if (row != NULL)
            gtk_widget_destroy(row);

          // release connection resources
          g_free(con);

          break;
        }
      }
    }

    // Free the list
    g_list_free(rows);
  }

  // destroy the dialog box
  gtk_widget_destroy(dialog);
}


/* menu item handler - add new connection */
void svHandleAddNewConnectionMenuItem ()
{
  app->addNewConnection = true;

  svCheckForNewConnectionAdd();
}


/* menu item handler - do screenshot of current vnc connection */
void svHandleScreenshotMenuItem ()
{
  Connection * con = svGetSelectedConnectionListConnection();

  if (con == NULL)
    return;

  // save screenshot to pixbuf
  GdkPixbuf * pic = vnc_display_get_pixbuf(VNC_DISPLAY(con->vncObj));

  // open save dialog
  GtkWidget * dialog;
  GtkFileChooser * chooser;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
  int res;
  char existingFilename[FILENAME_MAX] = {0};

  // Get the current time
  GDateTime * now = g_date_time_new_now_local();

  // Format the time as a human-readable string
  char * formattedTime = g_date_time_format(now, "%Y-%m-%d--%H-%M-%S");

  snprintf(existingFilename, 60, "spiritvnc-screenshot-%s.png", formattedTime);

  // Free resources
  g_free(formattedTime);
  g_date_time_unref(now);

  dialog = gtk_file_chooser_dialog_new("Save screenshot",
                                        GTK_WINDOW(app->mainWin),
                                        action,
                                        "_Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        "_Save",
                                        GTK_RESPONSE_ACCEPT,
                                        NULL);
  chooser = GTK_FILE_CHOOSER(dialog);

  gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
  gtk_file_chooser_set_current_name(chooser, existingFilename);

  res = gtk_dialog_run(GTK_DIALOG(dialog));

  if (res == GTK_RESPONSE_ACCEPT)
  {
    char * filename = gtk_file_chooser_get_filename(chooser);

    gdk_pixbuf_save(pic, filename, "png", NULL, "tEXt::Generator App", "spiritvncgtk", NULL);

    g_free(filename);
    g_object_unref(pic);
  }

  // destroy dialog
  gtk_widget_destroy(dialog);
}


/* create connection right-click menu */
void svConnectionRightClick (Connection * con)
{
  if (con == NULL)
    return;

  // right-click menu
  GtkWidget * rightMenu = gtk_menu_new();

  // connect menu item
  if (con->state != SV_STATE_CONNECTED && con->state != SV_STATE_WAITING)
  {
    GtkWidget * connect = gtk_menu_item_new_with_label("Connect");
    svSetTooltip(connect, "Attempts to start this connection to the remote host");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(connect), true);
    gtk_widget_set_sensitive(GTK_WIDGET(connect), true);
    gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), connect);
    g_signal_connect(connect, "activate", G_CALLBACK(svHandleConnectMenuItem), con);
  }

  // disconnect menu item
  if (con->state == SV_STATE_CONNECTED)
  {
    GtkWidget * disconnect = gtk_menu_item_new_with_label("Disconnect");
    svSetTooltip(disconnect, "Disconnects this connection from the remote host");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(disconnect), true);
    gtk_widget_set_sensitive(GTK_WIDGET(disconnect), true);
    gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), disconnect);
    g_signal_connect(disconnect, "activate", G_CALLBACK(svHandleDisconnectMenuItem), con);
  }

  // edit menu item
  GtkWidget * edit = gtk_menu_item_new_with_label("Edit");
  svSetTooltip(edit, "Opens this connection in the Edit Connection window");
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(edit), true);
  gtk_widget_set_sensitive(GTK_WIDGET(edit), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), edit);
  g_signal_connect(edit, "activate", G_CALLBACK(svHandleEditMenuItem), con);

  //// copy f12 macro menu item
  //GtkWidget * copymacro = gtk_menu_item_new_with_label("Copy F12 macro");
  //svSetTooltip(copymacro, "Stores this connection's F12 macro for use with a listening connection's F12 macro");
  //gtk_menu_item_set_use_underline(GTK_MENU_ITEM(copymacro), true);
  //gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), copymacro);
  //g_signal_connect(copymacro, "activate", G_CALLBACK(svHandleCopyF12MacroMenuItem), con);

  //// paste f12 macro menu item
  //if (con->state == SV_STATE_CONNECTED)
  //{
    //GtkWidget * pastemacro = gtk_menu_item_new_with_label("Copy F12 macro");
    //gtk_menu_item_set_use_underline(GTK_MENU_ITEM(pastemacro), true);
    //gtk_widget_set_sensitive(GTK_WIDGET(pastemacro), true);
    //gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), pastemacro);
    //g_signal_connect(pastemacro, "activate", G_CALLBACK(svHandlePasteF12MacroMenuItem), con);
  //}

  // delete menu item
  GtkWidget * delete = gtk_menu_item_new_with_label("Delete...");
  svSetTooltip(delete, "The app will ask you if you're sure you want to delete this connection");
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(delete), true);
  if (con->state != SV_STATE_CONNECTED && con->state != SV_STATE_WAITING)
    gtk_widget_set_sensitive(GTK_WIDGET(delete), true);
  else
    gtk_widget_set_sensitive(GTK_WIDGET(delete), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), delete);
  g_signal_connect(delete, "activate", G_CALLBACK(svHandleDeleteMenuItem), con);

  // only display custom commands submenu if any subcommands are enabled and have a label
  if ((con->customCmd1Enabled == true && strcmp(con->customCmd1Label->str, "") != 0) ||
    (con->customCmd2Enabled == true && strcmp(con->customCmd2Label->str, "") != 0) ||
    (con->customCmd3Enabled == true && strcmp(con->customCmd3Label->str, "") != 0))
  {
    // custom commands submenu
    GtkWidget * cmdSep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), cmdSep);

    // custom commands
    GtkWidget * cc = gtk_menu_item_new_with_label("Custom Commands");
    svSetTooltip(cc, "Custom Commands sub-menu");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cc), true);
    gtk_widget_set_sensitive(GTK_WIDGET(cc), true);
    gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), cc);

    // create submenu
    GtkWidget * customCommands = gtk_menu_new();

    // attach submenu to custom commands item
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(cc), customCommands);

    // custom command 1
    GtkWidget * cmdLbl1 = gtk_menu_item_new_with_label(con->customCmd1Label->str);
    svSetTooltip(cmdLbl1, "Runs Custom Command 1's command");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cmdLbl1), true);
    gtk_widget_set_sensitive(GTK_WIDGET(cmdLbl1), con->customCmd1Enabled);
    gtk_menu_shell_append(GTK_MENU_SHELL(customCommands), cmdLbl1);
    g_signal_connect(cmdLbl1, "activate", G_CALLBACK(svHandleCustomCommandMenuItem), con->customCmd1->str);

    // custom command 2
    GtkWidget * cmdLbl2 = gtk_menu_item_new_with_label(con->customCmd2Label->str);
    svSetTooltip(cmdLbl2, "Runs Custom Command 2's command");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cmdLbl2), true);
    gtk_widget_set_sensitive(GTK_WIDGET(cmdLbl2), con->customCmd2Enabled);
    gtk_menu_shell_append(GTK_MENU_SHELL(customCommands), cmdLbl2);
    g_signal_connect(cmdLbl2, "activate", G_CALLBACK(svHandleCustomCommandMenuItem), con->customCmd2->str);

    // custom command 3
    GtkWidget * cmdLbl3 = gtk_menu_item_new_with_label(con->customCmd3Label->str);
    svSetTooltip(cmdLbl3, "Runs Custom Command 3's command");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cmdLbl3), true);
    gtk_widget_set_sensitive(GTK_WIDGET(cmdLbl3), con->customCmd3Enabled);
    gtk_menu_shell_append(GTK_MENU_SHELL(customCommands), cmdLbl3);
    g_signal_connect(cmdLbl3, "activate", G_CALLBACK(svHandleCustomCommandMenuItem), con->customCmd3->str);
  }

  // show all the thingz
  gtk_widget_show_all(GTK_WIDGET(rightMenu));

  // pop up the menu
  gtk_menu_popup_at_pointer(GTK_MENU(rightMenu), NULL);
}


/* time out a connected connection if it doesn't initialize in time */
/* (run as a thread) */
gpointer svConnectionInitWaiter (void * data)
{
  Connection * con = (Connection *)data;

  if (con == NULL)
    return NULL;

  // wait for vnc connection timeout
  for (unsigned int i = 0; i < app->vncConnectWaitTime; i++)
  {
    // break out if connection becomes initialized
    if (con->state != SV_STATE_WAITING)
      break;

    sleep(1);
  }

  // if the connection isn't actually initialized (fully connected), close it
  if (con->state != SV_STATE_CONNECTED)
  {
    if (con->vncObj != NULL)
      vnc_display_close(VNC_DISPLAY(con->vncObj));

    // call server error handler directly
    svServerError(NULL, "Viewer failed to initialize in time", con);
  }

  return NULL;
}


/* handle vnc obj connection event */
void svServerConnected (GtkWidget * vncObj)
{
  Connection * con = svConnectionFromVNCObj(vncObj);

  if (con == NULL)
    return;

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Server connected (but not init'd) to '%s - %s'",
    con->name->str, con->address->str);
  svLog(logStr, true);

  // spawn initialize waiter thread
  GThread * initWaiter G_GNUC_UNUSED = g_thread_new("init-waiter", svConnectionInitWaiter, con);
}


/* handle vnc obj disconnection event */
void svServerDisconnected (GtkWidget * vncObj)
{
  Connection * con = svConnectionFromVNCObj(vncObj);

  if (con == NULL)
  {
    svLog("svServerDisconnected: con is null", false);
    return;
  }

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Server disconnected '%s - %s'", con->name->str, con->address->str);
  svLog(logStr, true);

  // if this is svnc, spawn ssh connection stop thread
  if (con->type == SV_TYPE_VNC_OVER_SSH)
    con->sshCloseThread = g_thread_new("ssh-closer", svSSHConnectionCloser, con);

  // * set disconnect state and icon *

  // manual disconnect
  if (con->state == SV_STATE_CONNECTED && con->disconnectType != SV_DISC_VNC_ERROR && con->disconnectType != SV_DISC_SSH_ERROR)
    con->state = SV_STATE_DISCONNECTED;

  // timeout disconnect
  else if (con->state == SV_STATE_WAITING && con->state != SV_STATE_TIMEOUT && con->disconnectType != SV_DISC_VNC_ERROR &&
    con->disconnectType != SV_DISC_SSH_ERROR)
    con->state = SV_STATE_TIMEOUT;

  // error
  else if (con->state == SV_STATE_ERROR || con->disconnectType == SV_DISC_VNC_ERROR || con->disconnectType == SV_DISC_SSH_ERROR)
    con->state = SV_STATE_ERROR;

  // set icon
  svSetIconFromConnectionName(con->name->str, con->state);

  con->vncObj = NULL;
}


/* handle vnc obj connection error event */
void svServerError (VncConnection * conn, const char * message, void * data)
{
  if (message == NULL)
    return;

  Connection * con = (Connection *)data;

  if (con == NULL)
    return;

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Server error '%s - %s': %s", con->name->str, con->address->str, message);
  svLog(logStr, true);

  // set state based on 'timeout' or 'timed out' string in error message
  if (strstr(message, "timeout") != NULL || strstr(message, "timed out") != NULL)
    con->state = SV_STATE_TIMEOUT;
  else
    con->state = SV_STATE_ERROR;

  svSetIconFromConnectionName(con->name->str, con->state);

  // set connection's last error message
  g_string_assign(con->lastErrorMessage, message);
  gtk_text_buffer_set_text(app->quickNoteLastErrorBuffer, con->lastErrorMessage->str, -1);
}


/* handle vnc obj initialize event (fully connected) */
void svServerInitialized (GtkWidget * vncObj)
{
  Connection * con = svConnectionFromVNCObj(vncObj);

  if (con == NULL)
    return;

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Server initialized (fully connected) '%s - %s'",
    con->name->str, con->address->str);
  svLog(logStr, true);

  con->state = SV_STATE_CONNECTED;
  svSetIconFromConnectionName(con->name->str, con->state);

  // add the vnc obj to the display stack
  gtk_stack_add_named(GTK_STACK(app->displayStack), vncObj, con->name->str);

  // set last connected time
  GDateTime * now = g_date_time_new_now_local();
  g_string_assign(con->lastConnectTime, g_date_time_format(now, "%H:%M:%S--%Y-%m-%d"));

  // set 'last connected' label text
  char strTime[50] = {0};
  snprintf(strTime, 49, "Last connected: %s", con->lastConnectTime->str);
  gtk_label_set_text(GTK_LABEL(app->quickNoteLastConnected), strTime);

  // only show newly-connected server if the listitem is selected
  char selectedRowText[1025] = {0};
  svSelectedRowText(selectedRowText);

  if (strcmp(con->name->str, selectedRowText) == 0)
  {
    gtk_widget_show(vncObj);
    gtk_stack_set_visible_child(GTK_STACK(app->displayStack), vncObj);

    // set tools menu items
    svSetToolsMenuItems(true);
  }
}


/* handle vnc obj authentication event */
void svServerAuthenticate (VncDisplay * display, GValueArray * credList)
{
  const char ** data = g_new0(const char *, credList->n_values);
  gsize outLen = 0;
  bool hasUsername = false;

  Connection * con = svConnectionFromVNCObj(GTK_WIDGET(display));

  // if connection is null, close the connection and get out
  if (con == NULL)
  {
    vnc_display_close(display);
    svLog("svServerAuthenticate - ERROR: con is null", false);
    return;
  }

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Server authentication '%s - %s'", con->name->str, con->address->str);
  svLog(logStr, true);

  // set vnc login username, if any
  char * loginUser = con->vncLoginUser->str;

  // decode vnc and login passwords, if any
  unsigned char * vncPass = g_base64_decode(con->vncPass->str, &outLen);
  unsigned char * loginPass = g_base64_decode(con->vncLoginPass->str, &outLen);

  // loop through requested credentials and set each
  for (unsigned int i = 0 ; i < credList->n_values ; i++)
  {
    GValue * cred = credList->values + 1;

    switch (g_value_get_enum(cred))
    {
      case VNC_DISPLAY_CREDENTIAL_USERNAME:
        hasUsername = true;
        data[i] = (char *)loginUser;
        svLog("Credential 'Username'", true);
        break;
      case VNC_DISPLAY_CREDENTIAL_PASSWORD:
      {
        if (hasUsername == true)
          data[i] = (char *)loginPass;
        else
          data[i] = (char *)vncPass;
        svLog("Credential 'Password'", true);
      }
      break;
      case VNC_DISPLAY_CREDENTIAL_CLIENTNAME:
          data[i] = "spiritvncgtk";
          svLog("Credential 'Client Name'", true);
          break;
      default:
          break;
    }
  }

  // loop through requested credentials and send data for each
  for (unsigned int i = 0 ; i < credList->n_values ; i++)
  {
    GValue * cred = credList->values + 1;

    if (data[i])
    {
      if (vnc_display_set_credential(VNC_DISPLAY(display), g_value_get_enum(cred), data[i]))
      {
        // log
        char logStr[FILENAME_MAX] = {0};
        snprintf(logStr, FILENAME_MAX - 1, "Failed to set credential type '%d'", g_value_get_enum(cred));
        svLog(logStr, false);

        vnc_display_close(VNC_DISPLAY(display));
        con->state = SV_STATE_DISCONNECTED;
      }
    }
    else
    {
      // log
      char logStr[FILENAME_MAX] = {0};
      snprintf(logStr, FILENAME_MAX - 1, "Unsupported credential type '%d'", g_value_get_enum(cred));
      svLog(logStr, false);

      vnc_display_close(VNC_DISPLAY(display));
      con->state = SV_STATE_DISCONNECTED;
    }
  }

  g_free(vncPass);
  g_free(loginPass);
  g_free(data);
}


/* saves the last selected connection's quicknote text */
/* (usually before switching to another connection) */
void svSavePreviousQuickNoteText (Connection * newConnection)
{
  // save previous quicknote before switching
  Connection * prevCon = app->selectedConnection;

  if (prevCon != NULL && prevCon != newConnection && strcmp(prevCon->name->str, "") != 0)
  {
    // get all text in quicknote buffer
    GtkTextIter start;
    GtkTextIter end;

    gtk_text_buffer_get_start_iter(app->quickNoteBuffer, &start);
    gtk_text_buffer_get_end_iter(app->quickNoteBuffer, &end);

    char * qText = gtk_text_buffer_get_text(app->quickNoteBuffer, &start, &end, FALSE);

    // set connection's quicknote text by base64 encoding it
    g_string_assign(prevCon->quickNote, g_base64_encode((const unsigned char *)qText, strlen(qText)));
  }
}


/* handle vnc obj 'server cut / copy (clipboard) text' event */
void svHandleServerClipboard (VncConnection * conn, const GString * text, void * data)
{
  printf("handle server clipboard\n");

  if (conn == NULL || data == NULL || text == NULL || text->len < 1)
    return;

  Connection * con = (Connection *)data;

  if (con == NULL)
    return;

  printf("server clipboard - len: %li\n", text->len);

  g_string_assign(con->clipboard, text->str);
}


/* create and start a new connection */
void svConnectionCreate (Connection * con)
{
  //  get out if state is CONNECTED or WAITING
  if (con == NULL || con->state == SV_STATE_CONNECTED || con->state == SV_STATE_WAITING)
    return;

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Starting new connection for '%s - %s'", con->name->str, con->address->str);
  svLog(logStr, true);

  // create a new vnc obj
  GtkWidget * vnc = vnc_display_new();

  if (vnc == NULL)
  {
    svLog("svConnectionCreate - CRITICAL: Unable to create a new vnc object!", false);
    return;
  }

  // set this connection's vnc obj
  con->vncObj = vnc;

  // connect vnc obj signals
  g_signal_connect(con->vncObj, "vnc-initialized", G_CALLBACK(svServerInitialized), NULL);
  g_signal_connect(con->vncObj, "vnc-connected", G_CALLBACK(svServerConnected), NULL);
  g_signal_connect(con->vncObj, "vnc-disconnected", G_CALLBACK(svServerDisconnected), NULL);
  g_signal_connect(con->vncObj, "vnc-auth-credential", G_CALLBACK(svServerAuthenticate), NULL);
  g_signal_connect(con->vncObj, "vnc-error", G_CALLBACK(svServerError), con);
  g_signal_connect(con->vncObj, "vnc-error", G_CALLBACK(svServerError), con);

  // don't handle clipboard for now -- possible gtk-vnc bug
  //g_signal_connect(con->vncObj, "vnc-server-cut-text", G_CALLBACK(svHandleServerClipboard), con);

  // change connection list icon
  svSetIconFromConnectionName(con->name->str, SV_STATE_WAITING);

  // reset connection variables
  con->state = SV_STATE_WAITING;
  con->disconnectType = SV_DISC_NONE;
  g_string_assign(con->lastErrorMessage, "");

  // process based on type
  switch (con->type)
  {
    case SV_TYPE_VNC:
    // just open the connection straight away
    svConnectionOpen(con);
    break;

    case SV_TYPE_VNC_OVER_SSH:
    // get a free local port to forward to
    con->sshLocalPort = svFindFreeTcpPort();

    // spawn ssh-related threads (because they block)
    con->sshThread = g_thread_new("ssh-runner", svCreateSSHConnection, con);
    con->sshMonitorThread = g_thread_new("ssh-monitor", svSSHMonitor, con);
    break;

    default:
    return;
  }
}


/* monitor ssh connection state and take appropriate action */
gpointer svSSHMonitor (gpointer data)
{
  Connection * con = (Connection *)data;

  if (con == NULL)
    return NULL;

  // wait for ssh connection
  for (unsigned int i = 0; i < app->sshConnectWaitTime; i++)
  {
    if (con->sshReady == true)
      break;

    sleep(1);
  }

  // deal with ssh connection timeout / failure
  if (con->sshReady == false)
  {
    // set connection's last error message
    g_string_assign(con->lastErrorMessage, "svSSHMonitor - Could not connect to SSH server");
    svLog(con->lastErrorMessage->str, false);

    // set last error message text view
    gtk_text_buffer_set_text(app->quickNoteLastErrorBuffer, con->lastErrorMessage->str, -1);

    // set connection variables
    con->sshReady = false;
    con->disconnectType = SV_DISC_SSH_ERROR;
    con->state = SV_STATE_ERROR;

    // set connection list row's icon
    svSetIconFromConnectionName(con->name->str, SV_STATE_ERROR);

    return NULL;
  }

  // if all seems well, wait to show
  for (unsigned int i = 0; i < app->sshShowWaitTime; i++)
    sleep(1);

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Attempting SSH connection open '%s - %s'",
    con->name->str, con->address->str);
  svLog(logStr, true);

  // attempt to connect to forwarded vnc
  svConnectionOpen(con);

  return NULL;
}


/* attempts to write closing commands to the ssh thread's stream */
gpointer svSSHConnectionCloser (gpointer data)
{
  Connection * con = (Connection *)data;

  if (con == NULL || con->sshStream == NULL)
    return NULL;

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Attempting to close SSH connection '%s - %s'",
    con->name->str, con->address->str);
  svLog(logStr, true);

  // send 'exit' and 'exit' control-char sequence
  fputs("\r\n", con->sshStream);
  fputs("exit\n\r\n", con->sshStream);
  fputs("~.\r\n", con->sshStream);
  fflush(con->sshStream);

  // close the ssh process stream
  pclose(con->sshStream);

  con->sshReady = false;

  return NULL;
}


/* ask connection to end */
/* (ssh cleanup is performed in svServerDisconnected) */
void svConnectionEnd (Connection * con)
{
  if (con == NULL)
    return;

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Ending connection '%s - %s'", con->name->str, con->address->str);
  svLog(logStr, true);

  // hide the vnc display widget
  gtk_widget_set_visible(con->vncObj, false);

  // set tools menu items
  svSetToolsMenuItems(false);

  // remove the vnc display widget from the display stack
  gtk_container_remove(GTK_CONTAINER(app->displayStack), con->vncObj);

  // close the vnc display connection
  vnc_display_close(VNC_DISPLAY(con->vncObj));
}


/* set tools menu connection-related items state */
void svSetToolsMenuItems(bool sState)
{
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->requestUpdate), sState);
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->send), sState);
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->sendEnteredKeys), sState);
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->sendCAD), sState);
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->sendCSE), sState);
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->screenshot), sState);
}


/* switch from one connection to another */
/* (usually from a single click on the connection listbox) */
void svConnectionSwitch (Connection * con)
{
  // ***### DO NOT CHECK CON FOR NULL HERE!!! ###***

  // disable tools menu items initially
  svSetToolsMenuItems(false);

  // save previous quicknote before switching
  svSavePreviousQuickNoteText(con);

  // there's no connection here, blank quicknote stuffs
  if (con == NULL)
  {
    gtk_label_set_text(GTK_LABEL(app->quickNoteLabel), "-");
    gtk_label_set_text(GTK_LABEL(app->quickNoteLastConnected), "-");
    gtk_text_buffer_set_text(app->quickNoteLastErrorBuffer, "", -1);
    gtk_text_buffer_set_text(app->quickNoteBuffer, "", -1);
  }
  else
  {
    // log
    char logStr[FILENAME_MAX] = {0};
    snprintf(logStr, FILENAME_MAX - 1, "Switching to connection '%s - %s'", con->name->str, con->address->str);
    svLog(logStr, true);

    // fill quicknote label, last error text and quicknote text
    gtk_label_set_text(GTK_LABEL(app->quickNoteLabel), con->name->str);
    char strTime[50] = {0};
    snprintf(strTime, 49, "Last connected: %s", con->lastConnectTime->str);
    gtk_label_set_text(GTK_LABEL(app->quickNoteLastConnected), strTime);
    gtk_text_buffer_set_text(app->quickNoteLastErrorBuffer, con->lastErrorMessage->str, -1);

    gsize outLen = 0;
    unsigned char * qNote = g_base64_decode(con->quickNote->str, &outLen);
    gtk_text_buffer_set_text(app->quickNoteBuffer, (const char *)qNote, -1);

    // set selected connection to passed connection
    app->selectedConnection = con;

    // show vnc obj if it's connected
    if (con->state == SV_STATE_CONNECTED && con->vncObj != NULL)
    {
      // show vnc display
      gtk_widget_set_visible(con->vncObj, true);
      gtk_stack_set_visible_child(GTK_STACK(app->displayStack), con->vncObj);

      // set tools menu items
      svSetToolsMenuItems(true);
    }
  }
}


/* attempts to connect the vnc obj to the server */
void svConnectionOpen (Connection * con)
{
  // set image quality
  switch (con->quality)
  {
    case SV_QUAL_LOW:
      vnc_display_set_depth(VNC_DISPLAY(con->vncObj), VNC_DISPLAY_DEPTH_COLOR_LOW);
      break;
    case SV_QUAL_MEDIUM:
      vnc_display_set_depth(VNC_DISPLAY(con->vncObj), VNC_DISPLAY_DEPTH_COLOR_MEDIUM);
      break;
    case SV_QUAL_FULL:
      vnc_display_set_depth(VNC_DISPLAY(con->vncObj), VNC_DISPLAY_DEPTH_COLOR_FULL);
      break;
    default:
      vnc_display_set_depth(VNC_DISPLAY(con->vncObj), VNC_DISPLAY_DEPTH_COLOR_DEFAULT);
  }

  // set lossy encoding
  vnc_display_set_lossy_encoding(VNC_DISPLAY(con->vncObj), con->lossyEncoding);

  // initiate connection
  switch (con->type)
  {
    case SV_TYPE_VNC:
      // connect vnc obj directly to remote host
      vnc_display_open_host(VNC_DISPLAY(con->vncObj), con->address->str, con->vncPort->str);
      break;

    case SV_TYPE_VNC_OVER_SSH:
      // set up local port string
      char localPortStr[10] = {0};
      snprintf(localPortStr, 9, "%i", con->sshLocalPort);

      // connect vnc obj to local forwarded port
      vnc_display_open_host(VNC_DISPLAY(con->vncObj), "127.0.0.1", localPortStr);
      break;

    default:
      return;
  }

  // vnc display is added to the stack in 'svServerInitialized' method
}


/* create ssh session and ssh forwarding */
/* (run as a thread due to blocking) */
gpointer svCreateSSHConnection (gpointer data)
{
  Connection * con = (Connection *)data;

  if (con == NULL)
    return NULL;

  // log
  char logStr[FILENAME_MAX] = {0};
  snprintf(logStr, FILENAME_MAX - 1, "Creating new SSH connection for '%s - %s'", con->name->str, con->address->str);
  svLog(logStr, true);

  #ifdef _WIN32
  // deal with annoying Windows terminal window issues
  AllocConsole();
  ShowWindow(GetConsoleWindow(), SW_HIDE);

  // set ssh check command
  GString * sshCheck = g_string_new(app->sshCommand->str);
  #else
  GString * sshCheck = g_string_new(NULL);
  g_string_printf(sshCheck, "%s > /dev/null", app->sshCommand->str);
  #endif

  // first check to see if the ssh command is working
  GError * gError = NULL;

  char ** chkCmd = g_strsplit(sshCheck->str, " ", -1);

  bool checkResult = g_spawn_sync(NULL,         // Working directory (NULL uses current directory)
                      chkCmd,      // Command to execute
                      NULL,         // Environment variables
                      G_SPAWN_DEFAULT, // Flags
                      NULL,         // Child setup function
                      NULL,         // User data for child setup
                      NULL,         // Standard output (NULL ignores it)
                      NULL,         // Standard error (NULL ignores it)
                      NULL, // Exit status
                      &gError);

  // deal with ssh command check failure
  if (checkResult != true)
  {
    // problemos
    //printf("ssh error: %i - %s\n", gError->code, gError->message);

    // log error
    char errStr[FILENAME_MAX] = {0};
    snprintf(errStr, FILENAME_MAX - 1, "svCreateSSHConnection - SSH command not working for connection '%s - %s'\nError: %s",
      con->name->str, con->address->str, gError->message);
    svLog(errStr, false);
    g_error_free(gError);

    // set connection's last error message
    g_string_assign(con->lastErrorMessage, "svCreateSSHConnection - SSH command check failed. Check SSH command or install.");
    gtk_text_buffer_set_text(app->quickNoteLastErrorBuffer, con->lastErrorMessage->str, -1);

    // set connection variables
    con->sshReady = false;
    con->disconnectType = SV_DISC_SSH_ERROR;
    con->state = SV_STATE_ERROR;

    // set connection list row's icon
    svSetIconFromConnectionName(con->name->str, SV_STATE_ERROR);

    return NULL;
  }

  // --- looks like we're okay ---
  GString * sshCommandLine = g_string_new(NULL);

  // build the command string for our popen() call
  g_string_printf(sshCommandLine, "%s %s@%s -t -t -p %s -o ConnectTimeout=%i -L %i:127.0.0.1:%s -i %s  > /dev/null",
    app->sshCommand->str, con->sshUser->str, con->address->str, con->sshPort->str, app->sshConnectWaitTime,
    con->sshLocalPort, con->vncPort->str, con->sshPrivKeyfile->str);

  // attempt to call the system's ssh client and open write stream
  con->sshStream = popen(sshCommandLine->str, "w");

  // handle failure
  if (con->sshStream == NULL)
  {
    // something -- happened - log the failure
    char errStr[FILENAME_MAX] = {0};
    snprintf(errStr, FILENAME_MAX - 1, "svCreateSSHConnection - SSH command failed for '%s - %s'", con->name->str, con->address->str);
    svLog(errStr, false);

    // set connection's last error message
    g_string_assign(con->lastErrorMessage, "SSH command failed. Check SSH command or install.");
    gtk_text_buffer_set_text(app->quickNoteLastErrorBuffer, con->lastErrorMessage->str, -1);

    // set connection variables
    con->sshReady = false;
    con->disconnectType = SV_DISC_SSH_ERROR;
    con->state = SV_STATE_ERROR;

    // set connection list row icon
    svSetIconFromConnectionName(con->name->str, SV_STATE_ERROR);
  }

  con->sshReady = true;

  if (gError != NULL)
    g_error_free(gError);

  return NULL;
}


/* handle app's activate event */
static void svAppActivate (GtkApplication * gtkApp, gpointer userData)
{
  svCreateGUI(gtkApp);
  svLog("--- App starting up ---", false);
}


/* main program */
int main (int argc, char ** argv)
{
  int status = 0;

  // give life to our app variable
  app = (App *)malloc(sizeof(App));

  // initialize the app struct
  svInitAppVars();

  // set application stuffs
  app->gApp = gtk_application_new ("org.will.brokenbourgh", 0);
  g_signal_connect(app->gApp, "activate", G_CALLBACK(svAppActivate), NULL);

  // run our app
  status = g_application_run(G_APPLICATION(app->gApp), 0, NULL);

  // unref after we're done
  g_object_unref(app->gApp);

  return status;
}
