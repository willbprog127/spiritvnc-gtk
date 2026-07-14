/*
 * spiritvnc.c - 2022-2026 Will Brokenbourgh
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
void svLog (const char * strIn, gboolean skipStdOut)
{
  // make time string
  GDateTime * now = g_date_time_new_now_local();
  char * nowStr = g_date_time_format(now, "%Y-%m-%d-%H:%M:%S");  //  <<<--- NO const char *

  // print to stdout if we aren't skipping or app->debugMode is true
  if (!skipStdOut || app->debugMode)
    // print to stdout
    printf("SpiritVNC-GTK: %s - %s\n", nowStr, strIn);

  // only log to file if set in options
  if (app->logToFile)
  {
    FILE * f = fopen(app->appLogFile->str, "a");  // opened in 'append' mode

    if (!f)
      return;

    // print to file
    fprintf(f, "%s: %s\n", nowStr, strIn);

    fflush(f);
    fclose(f);
  }

  g_free(nowStr);
  g_date_time_unref(now);
}


/* generic message dialog */
void svShowMessageDialog (const char * messageText)
{
  GtkWidget * dlg = gtk_message_dialog_new_with_markup(GTK_WINDOW(app->mainWin),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_MESSAGE_WARNING,
                                           GTK_BUTTONS_OK,
                                           NULL);

  gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dlg), messageText);

  gtk_dialog_run(GTK_DIALOG(dlg));
  gtk_widget_destroy(dlg);

  //gtk_window_present(GTK_WINDOW(app->mainWin));  // <<<-- don't use this here, use after calling this function
}


/* convert string to gboolean */
gboolean svStringToBool (const char * strIn)
{
  if (!strIn)
    return FALSE;

  char * strLow = g_utf8_strdown(strIn, -1);  //  <<<--- NO const char *

  gboolean result =
       strcmp(strLow, "1")    == 0 ||
       strcmp(strLow, "true") == 0 ||
       strcmp(strLow, "t")    == 0 ||
       strcmp(strLow, "yes")  == 0 ||
       strcmp(strLow, "y")    == 0;

  g_free(strLow);

  return result;
}


/* return connection that matches the passed vncObj */
Connection * svConnectionFromVNCObj (const GtkWidget * vncObj)
{
  // go through connection listbox and return correct connection
  GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

  for (GList * l = rows; l; l = l->next)
  {
    GtkListBoxRow * row = l->data;
    GtkWidget * box = gtk_bin_get_child(GTK_BIN(row));

    Connection * con = g_object_get_data(G_OBJECT(box), "con");
    if (con && con->vncObj == vncObj)
      return con;
  }

  g_list_free(rows);

  return NULL;
}


/* return a zero (0) or one (1) from a gboolean */
/* (justification: some systems don't always return a '1' for true and '0' for false) */
guint svIntFromBool (gboolean boolIn)
{
  if (boolIn)
    return 1;
  else
    return 0;
}


/* returns whatever text is selected in the connection list */
const char * svSelectedRowText ()
{
  GtkListBoxRow * selectedRow = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->serverList));
  if (!selectedRow)
    return NULL;

  GtkWidget * childBox = gtk_bin_get_child(GTK_BIN(selectedRow));
  if (!childBox)
    return NULL;

  GList * boxChildren = gtk_container_get_children(GTK_CONTAINER(childBox));

  GtkWidget * label = g_list_nth_data(boxChildren, 1);

  g_list_free(boxChildren);

  if (!label)
    return NULL;

  return gtk_label_get_text(GTK_LABEL(label));
}


/* initialize app variables */
void svInitAppVars ()
{
  if (!app)
  {
    printf("SpiritVNC-GTK: CRITICAL - app variable is NULL -- terminating app");
    exit(-1);
  }

  // important widgets
  app->mainWin = NULL;
  app->parent = NULL;
  app->serverList = NULL;
  app->toolsItems = g_new0(ToolsMenuItems, 1);

  // die if we can't create toolsItems
  if (!app->toolsItems)
  {
    printf("SpiritVNC-GTK - CRITICAL: toolsItems is NULL after creation -- terminating app");
    exit(-1);
  }

  app->connectionActionsWindow = NULL;

  // important objects
  app->selectedConnection = NULL;
  app->f12Storage = g_string_new(NULL);

  // app properties
  app->serverListWidth = 120;
  app->serverListWidthLast = app->serverListWidth;
  app->showTooltips = true;
  app->maximized = false;
  app->fullscreen = false;
  app->scanMode = false;
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
  app->listenMode = false;
  app->listenPort = 5500;
  app->scanTimerSource = 0;

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
}


/* initialize a connection object */
void svInitConnObject (Connection * con)
{
  if (!con)
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
  con->sshContinue = false;
  con->sshStream = NULL;
  con->sshStdIn = -1;
  con->clipboard = g_string_new(NULL);
}


/* set / unset hostlist items tooltips */
void svSetHostlistItemsTooltips ()
{
  GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

  for (GList * l = rows; l; l = l->next)
  {
    GtkListBoxRow * row = GTK_LIST_BOX_ROW(l->data);

    // get the rowBox
    GtkWidget * rowBox = gtk_bin_get_child(GTK_BIN(row));
    if (!rowBox)
      continue;

    // get the connection pointer from the box
    Connection * con = g_object_get_data(G_OBJECT(rowBox), "con");
    if (!con)
      continue;

    if (app->showTooltips)
    {
      GString * tipStr = g_string_new(NULL);
      const char * typeStr = NULL;

      if (con->type == SV_TYPE_VNC)
        typeStr = "VNC";
      else if (con->type == SV_TYPE_VNC_OVER_SSH)
        typeStr = "VNC over SSH";

      g_string_printf(tipStr, "<b>%s</b>\nType: %s\nAddress: %s\nLast connected: %s",
        con->name->str, typeStr, con->address->str, con->lastConnectTime->str);

      gtk_widget_set_tooltip_markup(rowBox, tipStr->str);
    }
    else
      gtk_widget_set_tooltip_text(rowBox, NULL);
    }

  g_list_free(rows);
}


/* sets a widget or menu-item's tooltip */
void svSetTooltip(GtkWidget * widget, const char * text)
{
  if (!widget || !text)
    return;

  // set or unset tooltip, based on app options
  if (app->showTooltips)
    gtk_widget_set_tooltip_text(widget, text);
  else
    gtk_widget_set_tooltip_text(widget, NULL);
}


/* handle send entered keystrokes window 'cancel' button */
void svHandleSendEnteredKeystrokesCancel (GtkButton * self, gpointer userData)
{
  SendKeysObj * obj = (SendKeysObj *)userData;

  if (!obj)
    return;

  // destroy send keystrokes window
  gtk_widget_destroy(obj->win);

  gtk_window_present(GTK_WINDOW(app->mainWin));

  g_free(obj);
}


/* handle send entered keystrokes window 'ok' button */
void svHandleSendEnteredKeystrokesSend (GtkButton * self, gpointer userData)
{
  SendKeysObj * obj = (SendKeysObj *)userData;
  if (!obj || !obj->con || !obj->con->vncObj || obj->con->state != SV_STATE_CONNECTED)
    return;

  char * text = NULL;

  // we're sending keys from the send keys window
  if (obj->type == SV_SENDKEYS_TYPE_ENTRY)
  {
    if (!obj->textView)
      return;

    GtkTextBuffer * tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(obj->textView));

    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(tb, &start);
    gtk_text_buffer_get_end_iter(tb, &end);

    text = gtk_text_buffer_get_text(tb, &start, &end, false);  //  <<<--- NO const char *
    if (text[0] == '\0')
    {
      g_free(text);
      return;
    }
  }
  // we're sending keys directly from another function
  else
  {
    if (!obj->textToSend || obj->textToSend->len == 0)
      return;

    // set text
    text = g_strdup(obj->textToSend->str);

    // free up gstring
    g_string_free(obj->textToSend, true);
  }

  size_t len = strlen(text);

  guint keys[len];

  for (size_t i = 0; i < len; i++)
    keys[i] = gdk_unicode_to_keyval(text[i]);

  vnc_display_send_keys(VNC_DISPLAY(obj->con->vncObj), keys, len);

  g_free(text);

  if (obj->win)
    gtk_widget_destroy(obj->win);

  gtk_window_present(GTK_WINDOW(app->mainWin));

  g_free(obj);
}


/* handle custom command output window 'ok' button */
void svHandleCommandOutputOk (GtkButton * self, gpointer userData)
{
  GtkWidget * outWin = (GtkWidget *)userData;
  if (!outWin)
    return;

  // destroy the command output window
  gtk_widget_destroy(outWin);

  gtk_window_present(GTK_WINDOW(app->mainWin));
}


/* handle the app options 'cancel' button */
void svHandleAppOptionsCancel (GtkButton * self, gpointer userData)
{
  AppOptions * opts = (AppOptions *)userData;
  if (!opts)
    return;

  GtkWidget * optsWin = (GtkWidget *)opts->optionsWin;
  if (!optsWin)
    return;

  // log
  svLog("Canceled app options editing", true);

  // destroy the app options window
  gtk_widget_destroy(optsWin);

  gtk_window_present(GTK_WINDOW(app->mainWin));

  g_free(opts);
}


/* handle the app options 'save' button */
void svHandleAppOptionsSave (GtkButton * self, gpointer userData)
{
  AppOptions * opts = (AppOptions *)userData;
  if (!opts)
    return;

  const GtkWidget * optsWin = (GtkWidget *)opts->optionsWin;
  if (!optsWin)
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
  svSetTooltip(app->listenImage, "Toggle listen mode");
  svSetTooltip(app->scanImage, "Toggle scan mode");
  svSetTooltip(app->addConnectionImage, "Add a new connection");

  // write out our config
  svConfigWrite();

  // destroy options window
  gtk_widget_destroy(opts->optionsWin);

  gtk_window_present(GTK_WINDOW(app->mainWin));

  g_free(opts);
}


/* handle the connection edit 'save' button */
void svHandleConnectionSettingsSave (GtkButton * self, gpointer userData)
{
  ConnectionSettings * settings = (ConnectionSettings *)userData;
  if (!settings || !settings->settingsWin)
    return;

  // set con object
  Connection * con = settings->con;
  if (!con)
    return;

  // log
  GString * logStr = g_string_new(NULL);
  g_string_printf(logStr, "Saving settings for '%s - %s'", con->name->str, con->address->str);
  svLog(logStr->str, true);
  g_string_free(logStr, true);

  // connection name
  const char * oldVal = (const char *)con->name->str;
  const char * nameVal = (const char *)gtk_entry_get_text(GTK_ENTRY(settings->connectionName));
  const char * addressVal = (const char *)gtk_entry_get_text(GTK_ENTRY(settings->remoteAddress));

  // **===== validate stuff first =====**

  // if no connection name, warn user
  if (nameVal[0] == '\0')
  {
    svShowMessageDialog("<b>'Connection name' cannot be empty</b>\n\nPlease enter a "
                                      "connection name then try saving again");

    gtk_window_present(GTK_WINDOW(settings->settingsWin));

    // focus remote address
    gtk_widget_grab_focus(settings->connectionName);

    return;
  }

  // if no host address, warn user
  if (addressVal[0] == '\0' && con->type != SV_TYPE_VNC_REVERSE)
  {
    svShowMessageDialog("<b>'Remote Address' cannot be empty</b>\n\nPlease enter a "
                                      "valid remote address then try saving again");

    gtk_window_present(GTK_WINDOW(settings->settingsWin));

    // focus remote address
    gtk_widget_grab_focus(settings->remoteAddress);

    return;
  }

  // set new name
  g_string_assign(con->name, nameVal);

  // update existing list item
  // if value changed, find server list row with old name and update
  if (strcmp(oldVal, nameVal) != 0)
  {
    GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

    // loop through each row
    for (GList * l = rows; l != NULL; l = l->next)
    {
      GtkWidget * row = GTK_WIDGET(l->data);

      // get box that holds status image and label
      GtkWidget * rowBox = gtk_bin_get_child(GTK_BIN(row));
      if (!rowBox)
        continue;

      // get box's children
      GList * boxChildren = gtk_container_get_children(GTK_CONTAINER(rowBox));

      GtkWidget * label = (GtkWidget *)g_list_nth_data(boxChildren, 1);

      g_list_free(boxChildren);

      // get label's text
      if (label && GTK_IS_LABEL(label))
      {
        const char * text = gtk_label_get_text(GTK_LABEL(label));

        // if label's text matches this edited connection's old
        // name, update it
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

  // connection group
  const char * groupVal = (char *)gtk_entry_get_text(GTK_ENTRY(settings->connectionGroup));

  // set new value
  if (groupVal[0] != '\0')
    g_string_assign(con->group, groupVal);
  else
    g_string_assign(con->group, "General");

  // connection address
  // set new value
  g_string_assign(con->address, addressVal);

  // connection f12 macro
  g_string_assign(con->f12Macro, gtk_entry_get_text(GTK_ENTRY(settings->f12Macro)));

  // don't change type for reverse vnc connections
  if (con->type != SV_TYPE_VNC_REVERSE)
  {
    // vnc choice radio button
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->vncChoice)))
      con->type = SV_TYPE_VNC;

    // vnc-over-ssh choice radio button
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings->svncChoice)))
      con->type = SV_TYPE_VNC_OVER_SSH;
  }

  // vnc port
  const char * vPortVal = (char *)gtk_entry_get_text(GTK_ENTRY(settings->vncPort));

  // set new value
  g_string_assign(con->vncPort, vPortVal);

  // vnc password
  const char * vPassVal = gtk_entry_get_text(GTK_ENTRY(settings->vncPassword));

  char * vPassDecoded = g_base64_encode((const unsigned char *)vPassVal, strlen(vPassVal));
  g_string_assign(con->vncPass, vPassDecoded);
  g_free(vPassDecoded);

  // vnc 'login' username
  g_string_assign(con->vncLoginUser, gtk_entry_get_text(GTK_ENTRY(settings->vncLoginUsername)));

  // vnc 'login' password
  const char * vLoginPassVal = gtk_entry_get_text(GTK_ENTRY(settings->vncLoginPassword));

  char * vLoginPassDecoded = g_base64_encode((const unsigned char *)vLoginPassVal, strlen(vLoginPassVal));
  g_string_assign(con->vncLoginPass, vLoginPassDecoded);
  g_free(vLoginPassDecoded);

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
  if (app->addNewConnection)
  {
    // check if a connection exists with the same name
    GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

    for (GList * l = rows; l; l = l->next)
    {
        GtkListBoxRow * row = l->data;
        GtkWidget * box = gtk_bin_get_child(GTK_BIN(row));

        const Connection * conTemp = g_object_get_data(G_OBJECT(box), "con");
        if (!conTemp)
            continue;

        // if a connection exists with this name, warn user
        if (strcmp(conTemp->name->str, con->name->str) == 0)
        {
          GtkWidget * dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(settings->settingsWin),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_CLOSE,
                                            "<b>Error: Connection name already exists</b>\n\nPlease change the "
                                              "connection's name then try saving again");
          gtk_dialog_run(GTK_DIALOG(dialog));
          gtk_widget_destroy(dialog);

          gtk_window_present(GTK_WINDOW(settings->settingsWin));

          // focus the connection name
          gtk_widget_grab_focus(settings->connectionName);

          return;
        }
    }

    gint rowIdx = -1;

    const char * lastGroup = NULL;
    gboolean myGroupFound = false;

    // go through connections glist and find last of this group
    for (GList * l = rows; l; l = l->next)
    {
      GtkListBoxRow * row = l->data;
      GtkWidget * box = gtk_bin_get_child(GTK_BIN(row));

      Connection * conTemp = g_object_get_data(G_OBJECT(box), "con");

      // break if we are at the end of this new connection's group
      if (conTemp)
      {
        // checking if we need to add an empty row before our new connection
        if (conTemp->group->len > 0 && strcmp(conTemp->group->str, con->group->str) == 0)
          myGroupFound = true;

        // get out of loop if we just left our group into another
        if (lastGroup && strcmp(conTemp->group->str, lastGroup) != 0 &&
          strcmp(lastGroup, con->group->str) == 0)
          break;

        lastGroup = conTemp->group->str;
      }

      rowIdx++;
    }

    g_list_free(rows);

    // insert into connections list at currently selected location or
    // the end of the list if rowIdx is -1

    // insert empty row before new connection (with new group)
    if (!myGroupFound)
      svInsertHostListRow("", rowIdx, NULL);

    // insert new connection
    svInsertHostListRow(con->name->str, rowIdx, con);

    app->addNewConnection = false;

    svSetHostlistItemsTooltips();
  }

  // destroy the connection settings window
  gtk_widget_destroy(settings->settingsWin);

  gtk_window_present(GTK_WINDOW(app->mainWin));

  // free the g_new'd settings
  g_free(settings);

  // update stuff if connected
  if (con->state == SV_STATE_CONNECTED)
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

    // set scaling
    vnc_display_set_scaling(VNC_DISPLAY(con->vncObj), con->scale);
    vnc_display_set_keep_aspect_ratio(VNC_DISPLAY(con->vncObj), TRUE);
  }
}


/* handles the connection editor cancel button */
void svHandleConnectionSettingsCancel (GtkButton* self, gpointer userData)
{
  ConnectionSettings * settings = (ConnectionSettings *)userData;

  if (!settings || !settings->settingsWin)
    return;

  // log
  svLog("Canceled connection editing", true);

  // destroy connection editing window
  gtk_widget_destroy(settings->settingsWin);

  gtk_window_present(GTK_WINDOW(app->mainWin));

  // free the g_new'd settings
  g_free(settings);

  app->addNewConnection = false;
}


/* handles the 'choose private key' button in the connection editor */
void svHandleConnectionSSHPrivKey (GtkButton * self, gpointer userData)
{
  ConnectionSettings * settings = (ConnectionSettings *)userData;
  if (!settings || !settings->settingsWin)
    return;

  GtkWidget * entry = settings->sshPrivateKey;
  if (!entry || !GTK_IS_ENTRY(entry))
    return;

  GtkWidget * dialog;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  gint res;

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
    char * filename;  //  <<<-- NO const char *
    GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);
    filename = gtk_file_chooser_get_filename(chooser);
    gtk_entry_set_text(GTK_ENTRY(entry), filename);
    g_free(filename);
  }

  // destroy the dialog window
  gtk_widget_destroy(dialog);

  gtk_window_present(GTK_WINDOW(app->mainWin));
}


/* create and display connection edit window */
void svShowAppOptionsWindow ()
{
  AppOptions * opts = g_new0(AppOptions, 1);
  if (!opts)
    return;

  // log
  svLog("Editing app options", true);

  // create edit window
  opts->optionsWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  // set window title
  gtk_window_set_title(GTK_WINDOW(opts->optionsWin), "SpiritVNC App Options");
  gtk_window_set_modal(GTK_WINDOW(opts->optionsWin), true);
  gtk_window_set_resizable(GTK_WINDOW(opts->optionsWin), false);
  gtk_window_set_transient_for(GTK_WINDOW(opts->optionsWin), GTK_WINDOW(app->mainWin));
  gtk_window_set_position(GTK_WINDOW(opts->optionsWin), GTK_WIN_POS_CENTER_ON_PARENT);

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

  guint rowNum = 1;

  // show tooltips
  GtkWidget * lblTooltips = gtk_label_new("Show tooltips");
  gtk_widget_set_halign(lblTooltips, GTK_ALIGN_END);
  opts->showTooltips = gtk_check_button_new();
  if (app->showTooltips)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(opts->showTooltips), true);
  svSetTooltip(opts->showTooltips, "Displays tooltips for most app items");

  gtk_grid_attach(GTK_GRID(optsPage), lblTooltips, 1, rowNum, 1, 1);
  gtk_grid_attach(GTK_GRID(optsPage), opts->showTooltips, 2, rowNum++, 1, 1);

  // log to file
  GtkWidget * lblLog = gtk_label_new("Log events to file");
  gtk_widget_set_halign(lblLog, GTK_ALIGN_END);
  opts->logToFile = gtk_check_button_new();
  svSetTooltip(opts->logToFile, "Logs most app events to a log file");

  if (app->logToFile)
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

  gtk_window_present(GTK_WINDOW(opts->optionsWin));

  // focus the first entry
  gtk_widget_grab_focus(opts->showTooltips);
}


/* handle send function key button */
void svHandleSendFunctionKeyButton (GtkWidget * unused, gpointer data)
{
  // get the connections glist item for this index
  Connection * con = svGetSelectedConnectionListConnection();
  if (!con || con->name->len == 0)
    return;

  guint key = GPOINTER_TO_UINT(data);

  guint keys[1];
  keys[0] = key;

  vnc_display_send_keys(VNC_DISPLAY(con->vncObj), keys, 1);
}


/* handle 'close' button on connection actions window */
void svHandleConnectionActionsCloseButton (GtkWidget * unused1, gpointer unused2)
{
  if (app->connectionActionsWindow)
  {
    gtk_widget_destroy(app->connectionActionsWindow);
    gtk_window_present(GTK_WINDOW(app->mainWin));
    app->connectionActionsWindow = NULL;
  }
}


/* handle esc key from connection actions window */
gboolean svHandleConnectionActionsCloseKey (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  // handle esc key
  if (event->keyval == GDK_KEY_Escape)
  {
    svHandleConnectionActionsCloseButton(NULL, NULL);
    return true;
  }

  return false;
}


/* create and display connection actions window */
void svShowConnectionActionsWindow ()
{
  if (app->connectionActionsWindow)
    return;

  // create connection actions window
  app->connectionActionsWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  // set window title
  gtk_window_set_title(GTK_WINDOW(app->connectionActionsWindow), "SpiritVNC Connection Actions");
  gtk_window_set_modal(GTK_WINDOW(app->connectionActionsWindow), true);
  //gtk_window_set_resizable(GTK_WINDOW(app->connectionActionsWindow), false);
  gtk_window_set_transient_for(GTK_WINDOW(app->connectionActionsWindow), GTK_WINDOW(app->mainWin));
  gtk_window_set_position(GTK_WINDOW(app->connectionActionsWindow), GTK_WIN_POS_CENTER_ON_PARENT);

  // parent grid
  GtkWidget * parentGrid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(parentGrid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(parentGrid), 10);
  gtk_container_set_border_width(GTK_CONTAINER(parentGrid), 10);
  gtk_container_add(GTK_CONTAINER(app->connectionActionsWindow), parentGrid);

  // send control-alt-delete
  GtkWidget * btnCAD = gtk_button_new_with_label("Send Ctrl+Alt+Del");
  g_signal_connect(btnCAD, "clicked", G_CALLBACK(svHandleSendCADMenuItem), NULL);
  gtk_grid_attach(GTK_GRID(parentGrid), btnCAD, 0, 0, 1, 1);

  // send control-alt-delete
  GtkWidget * btnCSE = gtk_button_new_with_label("Send Ctrl+Shift+Esc");
  g_signal_connect(btnCSE, "clicked", G_CALLBACK(svHandleSendCSEMenuItem), NULL);
  gtk_grid_attach(GTK_GRID(parentGrid), btnCSE, 1, 0, 1, 1);

  // send refresh request
  GtkWidget * btnRefresh = gtk_button_new_with_label("Send refresh request");
  g_signal_connect(btnRefresh, "clicked", G_CALLBACK(svHandleRequestUpdateMenuItem), NULL);
  gtk_grid_attach(GTK_GRID(parentGrid), btnRefresh, 0, 1, 1, 1);

  // send f8 key
  GtkWidget * btnF8 = gtk_button_new_with_label("Send F8 key");
  g_signal_connect(btnF8, "clicked", G_CALLBACK(svHandleSendFunctionKeyButton), GUINT_TO_POINTER(GDK_KEY_F8));
  gtk_grid_attach(GTK_GRID(parentGrid), btnF8, 1, 1, 1, 1);

  // send f9 key
  GtkWidget * btnF9 = gtk_button_new_with_label("Send F9 key");
  g_signal_connect(btnF9, "clicked", G_CALLBACK(svHandleSendFunctionKeyButton), GUINT_TO_POINTER(GDK_KEY_F9));
  gtk_grid_attach(GTK_GRID(parentGrid), btnF9, 0, 2, 1, 1);

  // send f11 key
  GtkWidget * btnF11 = gtk_button_new_with_label("Send F11 key");
  g_signal_connect(btnF11, "clicked", G_CALLBACK(svHandleSendFunctionKeyButton), GUINT_TO_POINTER(GDK_KEY_F11));
  gtk_grid_attach(GTK_GRID(parentGrid), btnF11, 1, 2, 1, 1);

  // send f12 key
  GtkWidget * btnF12 = gtk_button_new_with_label("Send F12 key");
  g_signal_connect(btnF12, "clicked", G_CALLBACK(svHandleSendFunctionKeyButton), GUINT_TO_POINTER(GDK_KEY_F12));
  gtk_grid_attach(GTK_GRID(parentGrid), btnF12, 0, 3, 1, 1);

  // send entered keys
  GtkWidget * btnKeys = gtk_button_new_with_label("Send entered keys...");
  g_signal_connect(btnKeys, "clicked", G_CALLBACK(svHandleSendEnteredKeystrokesMenuItem), NULL);
  gtk_grid_attach(GTK_GRID(parentGrid), btnKeys, 1, 3, 1, 1);

  // close button box
  GtkWidget * boxButtons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  // cancel
  GtkWidget * btnClose = gtk_button_new_with_label("Close");
  g_signal_connect(btnClose, "clicked", G_CALLBACK(svHandleConnectionActionsCloseButton), NULL);
  gtk_box_pack_end(GTK_BOX(boxButtons), btnClose, false, false, 3);
  //
  gtk_grid_attach(GTK_GRID(parentGrid), boxButtons, 1, 4, 1, 1);

  // show everything
  gtk_widget_show_all(app->connectionActionsWindow);

  // focus the window
  gtk_window_present(GTK_WINDOW(app->connectionActionsWindow));

  // add escape key handling to close window
  g_signal_connect(app->connectionActionsWindow, "key-press-event", G_CALLBACK(svHandleConnectionActionsCloseKey), NULL);
}


/* create and display connection edit window */
void svShowConnectionEditWindow (Connection * con)
{
  if (!con || con->name->len == 0)
    return;

  ConnectionSettings * settings = g_new0(ConnectionSettings, 1);
  if (!settings)
    return;

  // log
  GString * logStr = g_string_new(NULL);
  g_string_printf(logStr, "Editing connection '%s - %s'", con->name->str, con->address->str);
  svLog(logStr->str, true);
  g_string_free(logStr, true);

  settings->con = con;

  gboolean preventConnectedEditing = false;

  // don't allow editing of certain things if we're waiting or connected
  if (con->state == SV_STATE_WAITING || con->state == SV_STATE_CONNECTED)
    preventConnectedEditing = true;

  // create edit window
  settings->settingsWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_modal(GTK_WINDOW(settings->settingsWin), true);
  gtk_window_set_resizable(GTK_WINDOW(settings->settingsWin), false);
  gtk_window_set_transient_for(GTK_WINDOW(settings->settingsWin), GTK_WINDOW(app->mainWin));
  gtk_window_set_position(GTK_WINDOW(settings->settingsWin), GTK_WIN_POS_CENTER_ON_PARENT);

  // set window title
  GString * strTitle = g_string_new(NULL);
  g_string_printf(strTitle, "Editing '%s' - SpiritVNC", con->name->str);
  gtk_window_set_title(GTK_WINDOW(settings->settingsWin), strTitle->str);
  g_string_free(strTitle, true);

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
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->connectionName), false);
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
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->connectionGroup), false);
  gtk_entry_set_width_chars(GTK_ENTRY(settings->connectionGroup), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->connectionGroup), con->group->str);
  svSetTooltip(settings->connectionGroup, "The group this connection belongs to");

  gtk_grid_attach(GTK_GRID(vncPage), lblConGroup, 1, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->connectionGroup, 2, 2, 3, 1);

  // connection address
  GtkWidget * lblConAddr = gtk_label_new("Remote address");
  gtk_widget_set_halign(lblConAddr, GTK_ALIGN_END);
  settings->remoteAddress = gtk_entry_new();
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->remoteAddress), false);
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
  gboolean allowConnectionTypeChange = true;

  if (con->type == SV_TYPE_VNC_REVERSE || preventConnectedEditing)
    allowConnectionTypeChange = false;

  GtkWidget * boxVNCChoice = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);

  GtkWidget * lblVNCChoice = gtk_label_new("Server type");
  gtk_widget_set_halign(lblVNCChoice, GTK_ALIGN_END);

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCChoice, 1, 5, 1, 1);

  // - vnc choice group
  GSList * vncChoiceGroup = NULL;
  settings->vncChoice = gtk_radio_button_new_with_label(vncChoiceGroup, "VNC");
  gtk_widget_set_sensitive(settings->vncChoice, allowConnectionTypeChange);
  vncChoiceGroup = gtk_radio_button_get_group(GTK_RADIO_BUTTON(settings->vncChoice));
  settings->svncChoice = gtk_radio_button_new_with_label(vncChoiceGroup, "VNC over SSH");
  gtk_widget_set_sensitive(settings->svncChoice, allowConnectionTypeChange);
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
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->vncPort), false);
  gtk_entry_set_width_chars(GTK_ENTRY(settings->vncPort), 10);
  gtk_entry_set_text(GTK_ENTRY(settings->vncPort), con->vncPort->str);
  svSetTooltip(settings->vncPort, "The remote host's numbered VNC port");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCPort, 1, 6, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncPort, 2, 6, 3, 1);

  // vnc password
  GtkWidget * lblVNCPass = gtk_label_new("VNC password");
  gtk_widget_set_halign(lblVNCPass, GTK_ALIGN_END);
  settings->vncPassword = gtk_entry_new();
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->vncPassword), false);
  gtk_entry_set_width_chars(GTK_ENTRY(settings->vncPassword), 30);
  gtk_entry_set_visibility(GTK_ENTRY(settings->vncPassword), false);
  gsize vncPassOutLen = 0;
  unsigned char * vncPassTemp = g_base64_decode(con->vncPass->str, &vncPassOutLen);
  gtk_entry_set_text(GTK_ENTRY(settings->vncPassword), (const char *)vncPassTemp);
  g_free(vncPassTemp);
  svSetTooltip(settings->vncPassword, "The standard VNC password");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCPass, 1, 7, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncPassword, 2, 7, 3, 1);

  // vnc login username
  GtkWidget * lblVNCLoginName = gtk_label_new("VNC login username");
  gtk_widget_set_halign(lblVNCLoginName, GTK_ALIGN_END);
  settings->vncLoginUsername = gtk_entry_new();
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->vncLoginUsername), false);
  gtk_entry_set_width_chars(GTK_ENTRY(settings->vncLoginUsername), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->vncLoginUsername), con->vncLoginUser->str);
  svSetTooltip(settings->vncLoginUsername, "The username used with VNC 'login' authentication");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCLoginName, 1, 8, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncLoginUsername, 2, 8, 3, 1);

  // vnc login password
  GtkWidget * lblVNCLoginPass = gtk_label_new("VNC login password");
  gtk_widget_set_halign(lblVNCLoginPass, GTK_ALIGN_END);
  settings->vncLoginPassword = gtk_entry_new();
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->vncLoginPassword), false);
  gtk_entry_set_width_chars(GTK_ENTRY(settings->vncLoginPassword), 30);
  gtk_entry_set_visibility(GTK_ENTRY(settings->vncLoginPassword), false);
  gsize vncLoginPassOutLen = 0;
  unsigned char * vncLoginPassTemp = g_base64_decode(con->vncLoginPass->str, &vncLoginPassOutLen);
  gtk_entry_set_text(GTK_ENTRY(settings->vncLoginPassword), (const char *)vncLoginPassTemp);
  g_free(vncLoginPassTemp);
  svSetTooltip(settings->vncLoginPassword, "The password used with VNC 'login' authentication");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCLoginPass, 1, 9, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncLoginPassword, 2, 9, 3, 1);

  // vnc quality
  GtkWidget * lblVNCQuality = gtk_label_new("VNC quality");
  gtk_widget_set_halign(lblVNCQuality, GTK_ALIGN_END);
  settings->vncQuality = gtk_combo_box_text_new();
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->vncQuality), false);

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
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->vncLossyEncoding), false);
  if (con->lossyEncoding)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings->vncLossyEncoding), true);
  svSetTooltip(settings->vncLossyEncoding, "Enables the remote host's lossy encoding");

  gtk_grid_attach(GTK_GRID(vncPage), lblVNCLossy, 1, 11, 1, 1);
  gtk_grid_attach(GTK_GRID(vncPage), settings->vncLossyEncoding, 2, 11, 3, 1);

  // vnc scaling
  GtkWidget * lblVNCScaling = gtk_label_new("VNC scaling");
  gtk_widget_set_halign(lblVNCScaling, GTK_ALIGN_END);
  settings->vncScaling = gtk_check_button_new();
  if (con->scale)
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
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->sshUsername), false);
  gtk_entry_set_width_chars(GTK_ENTRY(settings->sshUsername), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->sshUsername), con->sshUser->str);
  svSetTooltip(settings->sshUsername, "The username used for the remote host's SSH server");

  gtk_grid_attach(GTK_GRID(sshPage), lblSSHName, 1, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(sshPage), settings->sshUsername, 2, 1, 3, 1);

  // ssh port
  GtkWidget * lblSSHPort = gtk_label_new("SSH port");
  gtk_widget_set_halign(lblSSHPort, GTK_ALIGN_END);
  settings->sshPort = gtk_entry_new();
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->sshPort), false);
  gtk_entry_set_width_chars(GTK_ENTRY(settings->sshPort), 10);
  gtk_entry_set_text(GTK_ENTRY(settings->sshPort), con->sshPort->str);
  svSetTooltip(settings->sshPort, "The remote host's numbered SSH server's port");

  gtk_grid_attach(GTK_GRID(sshPage), lblSSHPort, 1, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(sshPage), settings->sshPort, 2, 2, 3, 1);

  // ssh private key
  GtkWidget * lblSSHPrivKey = gtk_label_new("SSH private key (if any)");
  gtk_widget_set_halign(lblSSHPrivKey, GTK_ALIGN_END);
  settings->sshPrivateKey = gtk_entry_new();
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(settings->sshPrivateKey), false);
  gtk_entry_set_width_chars(GTK_ENTRY(settings->sshPrivateKey), 30);
  gtk_entry_set_text(GTK_ENTRY(settings->sshPrivateKey), con->sshPrivKeyfile->str);
  svSetTooltip(settings->sshPrivateKey, "The private key (if any) used to authenticate on the remote "
    "host's SSH server");

  GtkWidget * btnSSHPrivKey = gtk_button_new_with_label("...");
  if (preventConnectedEditing)
    gtk_widget_set_sensitive(GTK_WIDGET(btnSSHPrivKey), false);
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
  if (con->customCmd1Enabled)
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
  if (con->customCmd2Enabled)
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
  if (con->customCmd3Enabled)
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

  gtk_window_present(GTK_WINDOW(settings->settingsWin));

  // focus the first entry
  gtk_widget_grab_focus(settings->connectionName);
}


/* handle app->parent divider position change */
void svHandlePaneDividerChange (GtkPaned * paned, GtkScrollType * scroll_type, gpointer userData)
{
  if (!paned)
    return;

  // store handle position (basically 'list width')
  app->serverListWidth = gtk_paned_get_position(paned);
}


/* handle main window state changes */
void svHandleMainWinChange (GtkWidget * widget, GdkEventWindowState * event, gpointer userData)
{
  if (!widget || widget != app->mainWin)
    return;

  // set maximized value
  if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED)
    app->maximized = true;
  else
    app->maximized = false;

  // set fullscreen value
  if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)
    app->fullscreen = true;
  else
    app->fullscreen = false;
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
  svSetTooltip(app->toolsItems->scanMode, "Toggles timed scan mode");
  svSetTooltip(app->toolsItems->appOptions, "Displays the app preferences window");
  svSetTooltip(app->toolsItems->quit, "Closes all connections and quits the app");
}


/* check for, prompt and display new new connection add */
void svCheckForNewConnectionAdd ()
{
  // if nothing was added, prompt user to create a new connection
  if (!app->addNewConnection)
    return;

  // create and edit new connection
  Connection * con = g_new0(Connection, 1);
  if (!con)
    return;

  svInitConnObject(con);

  // set up new connection
  g_string_assign(con->name, "(new connection)");
  g_string_assign(con->group, "General");

  // show connection editor window
  svShowConnectionEditWindow(con);
}


/* handle reverse vnc connection */
gboolean svHandleReverseConnection (gpointer data)
{
  GSocket * sock = (GSocket *)data;
  if (!sock)
    return false;

  GSocket * client = g_socket_accept(sock, NULL, NULL);

  // create new connection object
  Connection * con = g_new0(Connection, 1);
  if (!con)
    return false;

  svInitConnObject(con);

  // make time string
  GDateTime * now = g_date_time_new_now_local();
  char * nowStr = g_date_time_format(now, "Listening-%Y%m%d%H%M%S");  //  <<<--- do NOT make const char *

  // set up new connection
  g_string_assign(con->name, nowStr);
  g_string_assign(con->group, "Listening");
  con->type = SV_TYPE_VNC_REVERSE;
  con->quality = SV_QUAL_MEDIUM;
  con->listenFd = g_socket_get_fd(client);
  con->scale = true;

  // create box, image and label for listbox row
  svInsertHostListRow(con->name->str, -1, con);

  svConnectionCreate(con);

  //printf("Reverse connection attempt\n");
  g_free(nowStr);
  g_date_time_unref(now);

  return true;
}


/* menu item handler - toggle listen mode */
void svHandleListenModeMenuItem ()
{
  app->listenMode = !app->listenMode;

  if (app->listenMode)
  {
    // change listen mode tools menu item text
    gtk_menu_item_set_label(GTK_MENU_ITEM(app->toolsItems->listenMode), "Disable _listen mode");
    //gtk_window_set_title(GTK_WINDOW(app->mainWin), "SpiritVNC (Listen Mode)");

    // change listen toolbutton to 'enabled' image
    if (app->listenImage)
    {
      gtk_image_clear(GTK_IMAGE(app->listenImage));
      gtk_image_set_from_resource(GTK_IMAGE(app->listenImage), "/com/spiritvnc/pngs/listen-enabled.png");
    }

    // create listening socket
    app->listenSocket = g_socket_new(G_SOCKET_FAMILY_IPV4,
                                 G_SOCKET_TYPE_STREAM,
                                 G_SOCKET_PROTOCOL_TCP,
                                 NULL);

    GInetAddress * addr = g_inet_address_new_any(G_SOCKET_FAMILY_IPV4);
    GSocketAddress * saddr = g_inet_socket_address_new(addr, app->listenPort);

    g_socket_bind(app->listenSocket, saddr, TRUE, NULL);
    g_socket_listen(app->listenSocket, NULL);

    // start listening
    app->listenSource = g_socket_create_source(app->listenSocket, G_IO_IN, NULL);
    g_source_set_callback(app->listenSource, svHandleReverseConnection, app->listenSocket, NULL);
    g_source_attach(app->listenSource, g_main_context_default());

  }
  else
  {
    // change listen mode tools menu item text
    gtk_menu_item_set_label(GTK_MENU_ITEM(app->toolsItems->listenMode), "Enable _listen mode");
    gtk_window_set_title(GTK_WINDOW(app->mainWin), "SpiritVNC");

    // change listen toolbutton to 'disabled' image
    if (app->listenImage)
    {
      gtk_image_clear(GTK_IMAGE(app->listenImage));
      gtk_image_set_from_resource(GTK_IMAGE(app->listenImage), "/com/spiritvnc/pngs/listen-disabled.png");
    }

    if (app->listenSource)
    {
      g_source_destroy(app->listenSource);
      g_source_unref(app->listenSource);
      app->listenSource = NULL;
    }

    if (app->listenSocket)
    {
      g_socket_close(app->listenSocket, NULL);
      g_object_unref(app->listenSocket);
      app->listenSocket = NULL;
    }
  }
}


/* handle window delete properly */
gboolean svMainWinDeleteEvent(GtkWidget * w, GdkEvent * e, gpointer u)
{
  svDoQuit();
  return false;
}


/* create the main app GUI */
void svCreateGUI (GtkApplication * gtkApp)
{
  if (app->mainWin)
    return;

  // create main window
  app->mainWin = gtk_application_window_new(gtkApp);
  //gtk_application_add_window(app->gApp, GTK_WINDOW(app->mainWin));
  gtk_window_set_title(GTK_WINDOW(app->mainWin), "SpiritVNC");
  gtk_window_set_default_size(GTK_WINDOW(app->mainWin), 1024, 720);
  g_signal_connect(app->mainWin, "delete-event", G_CALLBACK(svMainWinDeleteEvent), NULL);

  // go bye-bye if we can't even make the main window
  if (!app->mainWin)
  {
    svLog("SpiritVNC-GTK: Fatal error - main window is NULL. Terminating program", false);
    exit(-1);
  }

  // window icon
  GdkPixbuf * appIcon = gdk_pixbuf_new_from_resource("/com/spiritvnc/pngs/spiritvnc.png", NULL);
  if (appIcon)
  {
    gtk_window_set_default_icon(appIcon);
    g_object_unref(appIcon);
  }

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
  app->toolsItems->requestUpdate = upd;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(upd), true);
  gtk_widget_set_sensitive(GTK_WIDGET(upd), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), upd);
  g_signal_connect(upd, "activate", G_CALLBACK(svHandleRequestUpdateMenuItem), NULL);

  // ************ begin 'send' submenu *****************************

  // send submenu
  GtkWidget * ssm = gtk_menu_item_new_with_label("Send...");
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
  app->toolsItems->sendEnteredKeys = sks;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(sks), true);
  gtk_widget_set_sensitive(GTK_WIDGET(sks), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(sendMen), sks);
  g_signal_connect(sks, "activate", G_CALLBACK(svHandleSendEnteredKeystrokesMenuItem), NULL);

  // send ctrl+alt+del
  GtkWidget * cad = gtk_menu_item_new_with_label("Ctrl+_Alt+Del");
  app->toolsItems->sendCAD = cad;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cad), true);
  gtk_widget_set_sensitive(GTK_WIDGET(cad), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(sendMen), cad);
  g_signal_connect(cad, "activate", G_CALLBACK(svHandleSendCADMenuItem), NULL);

  // send ctrl+shift+esc
  GtkWidget * cse = gtk_menu_item_new_with_label("Ctrl+Shift+_Esc");
  app->toolsItems->sendCSE = cse;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cse), true);
  gtk_widget_set_sensitive(GTK_WIDGET(cse), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(sendMen), cse);
  g_signal_connect(cse, "activate", G_CALLBACK(svHandleSendCSEMenuItem), NULL);

  // ********* end of 'send' submenu ****************

  // screenshot
  GtkWidget * scr = gtk_menu_item_new_with_label("_Screenshot");
  app->toolsItems->screenshot = scr;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(scr), true);
  gtk_widget_set_sensitive(GTK_WIDGET(scr), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), scr);
  g_signal_connect(scr, "activate", G_CALLBACK(svHandleScreenshotMenuItem), NULL);

  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), gtk_separator_menu_item_new());

  // ********** end of connection-centric stuff

  // listen mode
  GtkWidget * lis = gtk_menu_item_new_with_label("Enable _listen mode");
  app->toolsItems->listenMode = lis;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(lis), true);
  gtk_widget_set_sensitive(GTK_WIDGET(lis), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), lis);
  g_signal_connect(lis, "activate", G_CALLBACK(svHandleListenModeMenuItem), NULL);

  // scan mode
  GtkWidget * scn = gtk_menu_item_new_with_label("Enable scan _mode");
  app->toolsItems->scanMode = scn;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(scn), true);
  gtk_widget_set_sensitive(GTK_WIDGET(scn), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), scn);
  g_signal_connect(scn, "activate", G_CALLBACK(svHandleScanModeMenuItem), NULL);

  // add new
  GtkWidget * add = gtk_menu_item_new_with_label("Add _new connection...");
  app->toolsItems->addNew = add;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(add), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), add);
  g_signal_connect(add, "activate", G_CALLBACK(svHandleAddNewConnectionMenuItem), NULL);

  // fullscreen
  GtkWidget * ful = gtk_menu_item_new_with_label("Toggle _fullscreen");
  app->toolsItems->fullscreen = ful;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(ful), true);
  gtk_widget_set_sensitive(GTK_WIDGET(ful), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), ful);
  g_signal_connect(ful, "activate", G_CALLBACK(svHandleFullscreenMenuItem), NULL);

  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), gtk_separator_menu_item_new());
  // ******* end of operations-centric stuff

  // app options
  GtkWidget * ao = gtk_menu_item_new_with_label("_Preferences");
  app->toolsItems->appOptions = ao;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(ao), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), ao);
  g_signal_connect(ao, "activate", G_CALLBACK(svShowAppOptionsWindow), NULL);

  // quit
  GtkWidget * qui = gtk_menu_item_new_with_label("_Quit");
  app->toolsItems->quit = qui;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(qui), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), qui);
  g_signal_connect(qui, "activate", G_CALLBACK(svDoQuit), NULL);

  // add submenu to tools
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(tools), submenu);

  //gtk_application_set_menubar(GTK_APPLICATION(gtkApp), G_MENU_MODEL(app->menuBar));

  // ################ TOOLS MENU - END #######################

  // help top menu
  GtkWidget * help = gtk_menu_item_new_with_label("_Help");
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(help), true);
  gtk_widget_set_sensitive(GTK_WIDGET(help), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(app->menuBar), help);

  GtkWidget * helpSub = gtk_menu_new();

  // about
  GtkWidget * abt = gtk_menu_item_new_with_label("_About");
  app->toolsItems->about = abt;
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(abt), true);
  gtk_menu_shell_append(GTK_MENU_SHELL(helpSub), abt);
  g_signal_connect(abt, "activate", G_CALLBACK(svShowAboutWindow), NULL);

  // add submenu to help
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(help), helpSub);

  // ################# PANES - START #################

  // server list -- nearly the heart of the program
  // * (keep this here, needed for config read) *
  app->serverList = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->serverList), GTK_SELECTION_BROWSE);
  g_signal_connect(app->serverList, "button-press-event", G_CALLBACK(svHandleConnectionListClicks), NULL);

  // read in the config file and fill the connection listbox + glist
  svConfigRead();

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
  //gtk_container_add(GTK_CONTAINER(app->mainWin), app->menuBar);

  // ############# toolbar area ###################################################
  GtkWidget * toolBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(leftBox), toolBar, false, false, 2);

  // listen toolbutton
  app->toolbuttonListen = gtk_event_box_new();
  g_signal_connect(G_OBJECT(app->toolbuttonListen), "button-press-event", G_CALLBACK(svHandleListenModeMenuItem), NULL);
  app->listenImage = gtk_image_new_from_resource("/com/spiritvnc/pngs/listen-disabled.png");
  svSetTooltip(app->listenImage, "Toggle listen mode");
  gtk_container_add(GTK_CONTAINER(app->toolbuttonListen), app->listenImage);
  gtk_box_pack_start(GTK_BOX(toolBar), app->toolbuttonListen, false, false, 3);

  // scan toolbutton
  app->toolbuttonScan = gtk_event_box_new();
  g_signal_connect(G_OBJECT(app->toolbuttonScan), "button-press-event", G_CALLBACK(svHandleScanModeMenuItem), NULL);
  app->scanImage = gtk_image_new_from_resource("/com/spiritvnc/pngs/scan-disabled.png");
  svSetTooltip(app->scanImage, "Toggle scan mode");
  gtk_container_add(GTK_CONTAINER(app->toolbuttonScan), app->scanImage);
  gtk_box_pack_start(GTK_BOX(toolBar), app->toolbuttonScan, false, false, 3);

  // add connection toolbutton
  app->addConnectionToolbutton = gtk_event_box_new();
  g_signal_connect(G_OBJECT(app->addConnectionToolbutton), "button-press-event", G_CALLBACK(svHandleAddNewConnectionMenuItem), NULL);
  app->addConnectionImage = gtk_image_new_from_resource("/com/spiritvnc/pngs/add-connection.png");
  svSetTooltip(app->addConnectionImage, "Add a new connection");
  gtk_container_add(GTK_CONTAINER(app->addConnectionToolbutton), app->addConnectionImage);
  gtk_box_pack_start(GTK_BOX(toolBar), app->addConnectionToolbutton, false, false, 3);

  // ############# toolbar area end ###################################################

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

  //gtk_window_present(GTK_WINDOW(app->mainWin));

  // if saved main window state was maximized, maximize the main window
  if (app->maximized)
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
gboolean svConfigCreateNew (gboolean createEmptyConfigFile)
{
  // create app's config folder, making parent folders as necessary
  gint result = g_mkdir_with_parents(app->appConfigDir->str, 0755);

  // error in folder creation
  if (result == -1)
  {
    GString * strErr = g_string_new(NULL);
    gint errCode = errno;
    g_string_printf(strErr, "svConfigCreateNew - Can't create config folder(s) - Error code: %i, %s",
      errCode, strerror(errCode));
    svLog(strErr->str, false);
    g_string_free(strErr, true);
    return false;
  }

  if (createEmptyConfigFile)
  {
    // only create a new empty file if one doesn't exist already
    if (!g_file_test(app->appConfigFile->str, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS))
    {
      svLog("svConfigCreateNew - Creating new empty config file", false);

      // --- attempt to write empty config file ---
      if (!g_file_set_contents(app->appConfigFile->str, "\n", -1, NULL))
      {
        svLog("svConfigCreateNew - Error: SpiritVNC could not write empty config file", false);
        return false;
      }
    }
  }

  return true;
}


/* insert a row into the connection listbox */
void svInsertHostListRow (const char * rowText, gint idx, Connection * con)
{
  if (!rowText)  // do NOT check con for NULL here, it's okay if it's null
    return;

  // box
  GtkWidget * rowBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
  if (!rowBox)
    return;

  // assign a connection to the row, if available
  g_object_set_data(G_OBJECT(rowBox), "con", con);

  // image
  GdkPixbuf * pb;

  if (rowText[0] != '\0')
    pb = gdk_pixbuf_new_from_resource("/com/spiritvnc/pngs/disconnected.png", NULL);
  else
    pb = gdk_pixbuf_new_from_resource("/com/spiritvnc/pngs/blank.png", NULL);

  GtkWidget * img = gtk_image_new_from_pixbuf(pb);

  // label
  GtkWidget * label = gtk_label_new(rowText);
  gtk_widget_set_halign(label, GTK_ALIGN_START);

  // add img and label to box
  gtk_box_pack_start(GTK_BOX(rowBox), img, false, false, 3);
  gtk_box_pack_start(GTK_BOX(rowBox), label, false, false, 0);

  gtk_widget_set_visible(img, true);
  gtk_widget_set_visible(label, true);
  gtk_widget_set_visible(rowBox, true);

  // add box to connections listbox
  gtk_list_box_insert(GTK_LIST_BOX(app->serverList), rowBox, idx);
}


/* read the config file and fill the connections listbox + glist */
void svConfigRead ()
{
  GError * readError = NULL;

  char * strIn = NULL;

  // check and create any missing config folders
  if (!svConfigCreateNew(true))
  {
    svLog("svConfigRead - Could not create new config dirs or file - quitting app", false);
    exit(-1);
  }

  // read config file contents into strIn
  if (!g_file_get_contents(app->appConfigFile->str, &strIn, NULL, &readError))
  {
    GString * errStr = g_string_new(NULL);

    if (readError)
      g_string_printf(errStr, "svConfigRead - Error opening config file\nError: %s", readError->message);
    else
      g_string_assign(errStr, "svConfigRead - Error opening config file - No error reported");

    svLog(errStr->str, false);
    g_string_free(errStr, true);
  }

  //printf("DEBUG: svConfigRead - readError pointer = %p\n", (void*)readError);

  if (readError)
    g_error_free(readError);

  // get out if strIn is null
  if (!strIn)
    return;

  // create initial con object
  Connection * con = g_new0(Connection, 1);
  svInitConnObject(con);

  char ** lines = g_strsplit(strIn, "\n", -1);  //  <<<--- do NOT make const char **

  g_free(strIn);

  GString * strLastGroup = g_string_new(NULL);

  // process each line
  for (char ** line = lines; *line != NULL; line++)
  {
    // process each line

    // split up property and value
    char ** propAndVal = g_strsplit(*line, "=", 2);

    // property and value must both be non-null
    if (!propAndVal[0] || !propAndVal[1])
    {
      g_strfreev(propAndVal);
      continue;
    }

    // trim whitespace (in-place)
    g_strstrip(propAndVal[0]);
    g_strstrip(propAndVal[1]);

    // assign prop and vals
    GString * strProp = g_string_new(propAndVal[0]);
    GString * strVal = g_string_new(propAndVal[1]);

    g_strfreev(propAndVal);

    //printf("Prop: %s, Val: %s\n", strProp->str, strVal->str);

    // ** start assigning values to properties **

    // ===== app settings =====

    // * serverListWidth *
    if (strcmp(strProp->str, "hostlistwidth") == 0)
    {
      app->serverListWidth = atoi(strVal->str);

      if (app->serverListWidth < 1)
        app->serverListWidth = 120;
    }

    // * showTooltips *
    if (strcmp(strProp->str, "showtooltips") == 0)
      app->showTooltips = svStringToBool(strVal->str);

    // * maximized *
    if (strcmp(strProp->str, "maximized") == 0)
      app->maximized = svStringToBool(strVal->str);

    // * log to file *
    if (strcmp(strProp->str, "logtofile") == 0)
      app->logToFile = svStringToBool(strVal->str);

    // * debug mode *
    if (strcmp(strProp->str, "debugmode") == 0)
      app->debugMode = svStringToBool(strVal->str);

    // * scan wait time *
    if (strcmp(strProp->str, "scantimeout") == 0)
      app->scanTimeout = atoi(strVal->str);

    // * vnc connect timeout *
    if (strcmp(strProp->str, "vnctimeout") == 0)
      app->vncConnectWaitTime = atoi(strVal->str);

    // * ssh command *
    if (strcmp(strProp->str, "sshcommand") == 0)
      g_string_assign(app->sshCommand, strVal->str);

    // * ssh connect timeout *
    if (strcmp(strProp->str, "sshtimeout") == 0)
      app->sshConnectWaitTime = atoi(strVal->str);

    // ===== individual connection settings =====

    // * connName *
    if (strcmp(strProp->str, "host") == 0)
    {
      // if there was a previous con object, add it
      // to the connections GList
      if (con->name && con->name->len > 0)
      {
        // create box, image and label for listbox row
        svInsertHostListRow(con->name->str, -1, con);

        app->addNewConnection = false;
      }

      // create new con object
      con = g_new0(Connection, 1);
      svInitConnObject(con);

      // connection name
      g_string_assign(con->name, strVal->str);
    }

    // * connGroup *
    if (strcmp(strProp->str, "group") == 0)
    {
      // add a separator if it's a new group
      if (strcmp(strVal->str, strLastGroup->str) != 0)
        svInsertHostListRow("", -1, NULL);

      g_string_assign(con->group, strVal->str);
      g_string_assign(strLastGroup, strVal->str);
    }

    // * address *
    if (strcmp(strProp->str, "address") == 0 || strcmp(strProp->str, "hostaddress") == 0)
      g_string_assign(con->address, strVal->str);

    // * type *
    if (strcmp(strProp->str, "type") == 0)
    {
      if (strcmp(strVal->str, "0") == 0 || strcmp(strVal->str, "v") == 0)
        con->type = SV_TYPE_VNC;
      else if (strcmp(strVal->str, "1") == 0 || strcmp(strVal->str, "s") == 0)
        con->type = SV_TYPE_VNC_OVER_SSH;
      else
        con->type = SV_TYPE_VNC;
    }

    // * vncPort *
    if (strcmp(strProp->str, "vncport") == 0)
      g_string_assign(con->vncPort, strVal->str);

    // * vncPass *
    if (strcmp(strProp->str, "vncpass") == 0)
      g_string_assign(con->vncPass, strVal->str);

    // * vncLoginUser *
    if (strcmp(strProp->str, "vncloginuser") == 0)
      g_string_assign(con->vncLoginUser, strVal->str);

    // * vncLoginPass *
    if (strcmp(strProp->str, "vncloginpass") == 0)
      g_string_assign(con->vncLoginPass, strVal->str);

    // * scaling *
    if (strcmp(strProp->str, "scale") == 0)
      con->scale = svStringToBool(strVal->str);

    // * showRemoteCursor *
    if (strcmp(strProp->str, "showremotecursor") == 0)
      con->showRemoteCursor = svStringToBool(strVal->str);

    // * imageQuality *
    if (strcmp(strProp->str, "quality") == 0)
    {
      if (strcmp(strVal->str, "0") == 0)
        con->quality = SV_QUAL_LOW;
      else if (strcmp(strVal->str, "1") == 0 || strcmp(strVal->str, "5") == 0)
        con->quality = SV_QUAL_MEDIUM;
      else if (strcmp(strVal->str, "2") == 0 || strcmp(strVal->str, "9") == 0)
        con->quality = SV_QUAL_FULL;
      else
        con->quality = SV_QUAL_DEFAULT;
    }

    // * lossy encoding *
    if (strcmp(strProp->str, "lossyencoding") == 0)
      con->lossyEncoding = svStringToBool(strVal->str);

    // * sshPort *
    if (strcmp(strProp->str, "sshport") == 0)
      g_string_assign(con->sshPort, strVal->str);

    // * sshPrivKeyfile *
    if (strcmp(strProp->str, "sshkeyprivate") == 0)
      g_string_assign(con->sshPrivKeyfile, strVal->str);

    // * sshUser *
    if (strcmp(strProp->str, "sshuser") == 0)
      g_string_assign(con->sshUser, strVal->str);

    // * f12Macro *
    if (strcmp(strProp->str, "f12macro") == 0)
      g_string_assign(con->f12Macro, strVal->str);

    // * quicknote *
    if (strcmp(strProp->str, "quicknote") == 0)
      g_string_assign(con->quickNote, strVal->str);

    // * custom command 1 enabled *
    if (strcmp(strProp->str, "customcmd1enabled") == 0 || strcmp(strProp->str, "customcommand1enabled") == 0)
      con->customCmd1Enabled = svStringToBool(strVal->str);

    // * custom command 1 label *
    if (strcmp(strProp->str, "customcmd1label") == 0 || strcmp(strProp->str, "customcommand1label") == 0)
      g_string_assign(con->customCmd1Label, strVal->str);

    // * custom command 1 *
    if (strcmp(strProp->str, "customcmd1") == 0 || strcmp(strProp->str, "customcommand1") == 0)
      g_string_assign(con->customCmd1, strVal->str);

    // * custom command 2 enabled *
    if (strcmp(strProp->str, "customcmd2enabled") == 0 || strcmp(strProp->str, "customcommand2enabled") == 0)
      con->customCmd2Enabled = svStringToBool(strVal->str);

    // * custom command 2 label *
    if (strcmp(strProp->str, "customcmd2label") == 0 || strcmp(strProp->str, "customcommand2label") == 0)
      g_string_assign(con->customCmd2Label, strVal->str);

    // * custom command 2 *
    if (strcmp(strProp->str, "customcmd2") == 0 || strcmp(strProp->str, "customcommand2") == 0)
      g_string_assign(con->customCmd2, strVal->str);

    // * custom command 3 enabled *
    if (strcmp(strProp->str, "customcmd3enabled") == 0 || strcmp(strProp->str, "customcommand3enabled") == 0)
      con->customCmd3Enabled = svStringToBool(strVal->str);

    // * custom command 3 label *
    if (strcmp(strProp->str, "customcmd3label") == 0 || strcmp(strProp->str, "customcommand3label") == 0)
      g_string_assign(con->customCmd3Label, strVal->str);

    // * custom command 3 *
    if (strcmp(strProp->str, "customcmd3") == 0 || strcmp(strProp->str, "customcommand3") == 0)
      g_string_assign(con->customCmd3, strVal->str);

    // * last connect time *
    if (strcmp(strProp->str, "lastconnecttime") == 0)
      g_string_assign(con->lastConnectTime, strVal->str);

    // free up gstrings
    g_string_free(strProp, true);
    g_string_free(strVal, true);
  }

  // free up stuffz
  g_string_free(strLastGroup, true);
  g_strfreev(lines);

  // add last con item, if not null
  if (con && con->name->len > 0)
  {
    svInsertHostListRow(con->name->str, -1, con);

    app->addNewConnection = false;
  }
  else
    svFreeConnObject(con);

  svInsertHostListRow("", -1, NULL);

  // set or unset hostlist item tooltips
  svSetHostlistItemsTooltips();
}


/* write the configuration file */
void svConfigWrite ()
{
  // no re-entrance
  static gboolean inConfigWrite = false;

  if (inConfigWrite)
    return;

  // set non-re-entrance flag
  inConfigWrite = true;

  // if it's not possible to create a config directory, get out
  if (!svConfigCreateNew(false))
  {
    svLog("svConfigWrite - Could not create new config dirs or file", false);
    inConfigWrite = false;
    return;
  }

  GString * outStr = g_string_new(NULL);

  // header
  g_string_append(outStr, "# SpiritVNC - GTK 3 version\n");
  g_string_append(outStr, "# 2022-2026 Will Brokenbourgh - to God be the glory!\n#\n");

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
  // go through connection list and add each connection's settings
  GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

  for (GList * l = rows; l; l = l->next)
  {
    GtkListBoxRow * row = l->data;
    GtkWidget * box = gtk_bin_get_child(GTK_BIN(row));

    Connection * con = g_object_get_data(G_OBJECT(box), "con");

    if (!con || con->name->len == 0 || con->type == SV_TYPE_VNC_REVERSE)
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
  if (!g_file_set_contents(app->appConfigFile->str, outStr->str, -1, NULL))
    svLog("svConfigWrite - Error: SpiritVNC could not write config file", false);

  g_list_free(rows);
  g_string_free(outStr, true);

  // set no-re-entrance flag
  inConfigWrite = false;
}


/* sets a connection's icon in the connection list */
void svSetIconFromConnectionName (const char * text, guint state)
{
  GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

  // loop through each row
  for (GList * l = rows; l != NULL; l = l->next)
  {
    GtkWidget * row = GTK_WIDGET(l->data);

    // get box that holds status image and label
    GtkWidget * rowBox = gtk_bin_get_child(GTK_BIN(row));
    if (!rowBox)
      continue;

    // get box's children
    GList * boxChildren = gtk_container_get_children(GTK_CONTAINER(rowBox));

    GtkWidget * img = (GtkWidget *)g_list_nth_data(boxChildren, 0);
    GtkWidget * label = (GtkWidget *)g_list_nth_data(boxChildren, 1);

    g_list_free(boxChildren);

    // get label's text, if any
    const char * lText = gtk_label_get_text(GTK_LABEL(label));

    if (!lText)
      continue;

    // if the label text matches, modify the icon
    if (strcmp(lText, text) == 0)
    {
      if (img)
      {
        gtk_image_clear(GTK_IMAGE(img));

        // image
        GdkPixbuf * pb;

        switch (state)
        {
          case SV_STATE_DISCONNECTED:
            pb = gdk_pixbuf_new_from_resource("/com/spiritvnc/pngs/disconnected.png", NULL);
            break;

          case SV_STATE_CONNECTED:
            pb = gdk_pixbuf_new_from_resource("/com/spiritvnc/pngs/connected.png", NULL);
            break;

          case SV_STATE_WAITING:
            pb = gdk_pixbuf_new_from_resource("/com/spiritvnc/pngs/connecting.png", NULL);
            break;

          case SV_STATE_TIMEOUT:
            pb = gdk_pixbuf_new_from_resource("/com/spiritvnc/pngs/noconnect.png", NULL);
            break;

          case SV_STATE_ERROR:
            pb = gdk_pixbuf_new_from_resource("/com/spiritvnc/pngs/disconnected_error.png", NULL);
            break;

          default:
            pb = gdk_pixbuf_new_from_resource("/com/spiritvnc/pngs/blank.png", NULL);
        }

        // set the icon
        gtk_image_set_from_pixbuf(GTK_IMAGE(img), pb);

      }

      return;
    }
  }

  g_list_free(rows);
}


/* sets a connection's text in the connection list */
void svSetTextFromConnectionName (const char * currentText, const char * newText)
{
  if (!currentText || !newText)
    return;

  GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

  // loop through each row
  for (GList * l = rows; l != NULL; l = l->next)
  {
    GtkWidget * row = GTK_WIDGET(l->data);

    // get box that holds status image and label
    GtkWidget * rowBox = gtk_bin_get_child(GTK_BIN(row));

    if (!rowBox)
      continue;

    // get box's children
    GList * boxChildren = gtk_container_get_children(GTK_CONTAINER(rowBox));

    GtkWidget * label = (GtkWidget *)g_list_nth_data(boxChildren, 1);

    g_list_free(boxChildren);

    // get label's text, if any
    const char * lText = gtk_label_get_text(GTK_LABEL(label));

    if (!lText)
      continue;

    // if the label text matches, change to new text
    if (strcmp(lText, currentText) == 0)
    {
      if (label)
        gtk_label_set_label(GTK_LABEL(label), newText);

      return;
    }
  }

  g_list_free(rows);
}


/* closes all connections (typically when exiting program) */
void svEndAllConnections ()
{
  // end all viewers
  GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

  for (GList * l = rows; l; l = l->next)
  {
    GtkListBoxRow * row = l->data;
    GtkWidget * box = gtk_bin_get_child(GTK_BIN(row));

    Connection * con = g_object_get_data(G_OBJECT(box), "con");

    if (con && con->state == SV_STATE_CONNECTED && con->vncObj)
      vnc_display_close(VNC_DISPLAY(con->vncObj));
  }

  g_list_free(rows);
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
gint svFindFreeTcpPort ()
{
  gint nSock = 0;
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

  guint clickCount = 0;
  guint button = 0;

  // set last button number
  if (!gdk_event_get_button(event, &button))
    return false;

  // set last click count
  if (!gdk_event_get_click_count(event, &clickCount))
    return false;

  double x = 0;
  double y = 0;

  // get the row's internal name from the mouse's y position
  if (!gdk_event_get_coords(event, &x, &y))
    return false;

  // hide any active display if list is single-clicked
  if (button != GDK_BUTTON_SECONDARY && clickCount == 1)
  {
    GtkWidget * currentDisplay = gtk_stack_get_visible_child(GTK_STACK(app->displayStack));

    if (currentDisplay)
      gtk_widget_hide(currentDisplay);
  }

  // get the clicked row
  GtkListBoxRow * row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(widget), y);
  if (!row)
  {
    //printf("DEBUG: row is null in svHandleConnectionListClicks\n");
    return false;
  }

  // get box that holds status image and label
  GtkWidget * rowBox = gtk_bin_get_child(GTK_BIN(row));
  if (!rowBox)
    return false;

  // get connection from attached rowBox data
  Connection * con = g_object_get_data(G_OBJECT(rowBox), "con");

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


/* return the Connection * of the currently-selected connection listbox item */
Connection * svGetSelectedConnectionListConnection ()
{
  // figure out which connection is selected
  GtkListBoxRow * selectedRow = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->serverList));
  if (!selectedRow)
    return NULL;

  GtkWidget * rowBox = gtk_bin_get_child(GTK_BIN(selectedRow));
  if (!rowBox)
    return NULL;

  return (Connection *)g_object_get_data(G_OBJECT(rowBox), "con");
}


/* menu item handler - send update request to currently-selected server */
void svHandleRequestUpdateMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  // get the connections glist item for this index
  Connection * con = svGetSelectedConnectionListConnection();

  if (!con || con->name->len == 0)
    return;

  vnc_display_request_update(VNC_DISPLAY(con->vncObj));
}


/* menu item handler - send Ctrl+Alt+Del to currently-selected server */
void svHandleSendCADMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  // get the connections glist item for this index
  Connection * con = svGetSelectedConnectionListConnection();
  if (!con || con->name->len == 0)
    return;

  guint keys[] = {
    GDK_KEY_Control_L,
    GDK_KEY_Alt_L,
    GDK_KEY_Delete
  };

  vnc_display_send_keys(VNC_DISPLAY(con->vncObj), keys, sizeof(keys) / sizeof(keys[0]));
}


/* menu item handler - send Ctrl+Shift+Esc to currently-selected server */
void svHandleSendCSEMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  // get the connections glist item for this index
  Connection * con = svGetSelectedConnectionListConnection();

  if (!con || con->name->len == 0)
    return;

  guint keys[] = {
    GDK_KEY_Control_L,
    GDK_KEY_Shift_L,
    GDK_KEY_Escape
  };

  vnc_display_send_keys(VNC_DISPLAY(con->vncObj), keys, sizeof(keys) / sizeof(keys[0]));
}


/* menu item handler - shows 'send keys' window */
void svHandleSendEnteredKeystrokesMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  SendKeysObj * obj = g_new0(SendKeysObj, 1);
  if (!obj)
    return;

  // get the connections glist item for this index
  Connection * con = svGetSelectedConnectionListConnection();
  if (!con || con->name->len == 0)
    return;

  // create output window
  GtkWidget * sendKeysWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_modal(GTK_WINDOW(sendKeysWin), true);
  gtk_window_set_transient_for(GTK_WINDOW(sendKeysWin), GTK_WINDOW(app->mainWin));
  gtk_window_set_position(GTK_WINDOW(sendKeysWin), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_modal(GTK_WINDOW(sendKeysWin), true);
  gtk_window_set_resizable(GTK_WINDOW(sendKeysWin), false);

  // set window title
  GString * titleStr = g_string_new(NULL);
  g_string_printf(titleStr, "Send keys to '%s' - SpiritVNC", con->name->str);
  gtk_window_set_title(GTK_WINDOW(sendKeysWin), titleStr->str);
  g_string_free(titleStr, true);

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
  obj->type = SV_SENDKEYS_TYPE_ENTRY;
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

  gtk_window_present(GTK_WINDOW(sendKeysWin));
}


/* menu item handler - run custom command */
void svHandleCustomCommandMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  const char * cmd = (const char *)userData;
  if (!cmd)
    return;

  GError * gError = NULL;
  char * stdOut = NULL;

  // run the command synchronous, will block
  if (!g_spawn_command_line_sync(cmd, &stdOut, NULL, NULL, &gError))
  {
    GtkWidget * dialog = gtk_message_dialog_new_with_markup(GTK_WINDOW(app->mainWin),
                                      GTK_DIALOG_DESTROY_WITH_PARENT,
                                      GTK_MESSAGE_ERROR,
                                      GTK_BUTTONS_CLOSE,
                                      "<b>Error running custom command</b>\n\n%s",
                                      gError->message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    gtk_window_present(GTK_WINDOW(app->mainWin));

    if (gError)
      g_error_free(gError);

     return;
  }
  else
  {
    // show command output if not null or empty
    if (stdOut && stdOut[0] != '\0')
    {
      // create output window
      GtkWidget * outWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_window_set_modal(GTK_WINDOW(outWin), true);

      // set window title
      gtk_window_set_title(GTK_WINDOW(outWin), "Custom command output - SpiritVNC");
      gtk_window_set_modal(GTK_WINDOW(outWin), true);
      gtk_window_set_resizable(GTK_WINDOW(outWin), false);
      gtk_window_set_transient_for(GTK_WINDOW(outWin), GTK_WINDOW(app->mainWin));
      gtk_window_set_position(GTK_WINDOW(outWin), GTK_WIN_POS_CENTER_ON_PARENT);

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

      // command label (to show command that was executed)
      GtkWidget * lblCmd = gtk_label_new("");
      gtk_widget_set_halign(lblCmd, GTK_ALIGN_START);
      GString * cmdString = g_string_new(NULL);
      g_string_printf(cmdString, "Command: '%s'", cmd);
      gtk_label_set_text(GTK_LABEL(lblCmd), cmdString->str);
      g_string_free(cmdString, true);
      gtk_grid_attach(GTK_GRID(grid), lblCmd, 1, 2, 1, 1);

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
      GtkWidget * btnOk = gtk_button_new_with_label("OK");
      gtk_widget_set_size_request(btnOk, 110, -1);
      g_signal_connect(btnOk, "clicked", G_CALLBACK(svHandleCommandOutputOk), outWin);

      gtk_box_pack_end(GTK_BOX(boxButtons), btnOk, false, false, 3);
      //
      gtk_container_add(GTK_CONTAINER(boxParent), boxButtons);

      // show everything
      gtk_widget_show_all(outWin);

      gtk_window_present(GTK_WINDOW(outWin));
    }

    if (stdOut)
      g_free(stdOut);
  }
}


/* menu item handler - call connection start */
void svHandleConnectMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;
  if (!con)
    return;

  svConnectionCreate(con);
}


/* menu item handler - call connection end */
void svHandleDisconnectMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;
  if (!con)
    return;

  svConnectionEnd(con);
}


/* menu item handler - call connection editor */
void svHandleEditMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;
  if (!con)
    return;

  svShowConnectionEditWindow(con);
}

/* menu item handler - call connection editor */
void svHandleSyncClipboardMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;
  if (!con || !con->clipboard || con->clipboard->len < 1)
    return;

  // set local keyboard
  GtkClipboard * cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  if (!cb)
    return;

  gtk_clipboard_set_text(cb, con->clipboard->str, -1);
}


/* menu item handler - get right-clicked connection's f12 macro (NOT on clipboard!) */
void svHandleF12GetMacroMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;

  if (!con)
    return;

  g_string_assign(app->f12Storage, con->f12Macro->str);
}


/* menu item handler - set right-clicked connection's f12 macro (NOT from clipboard!) */
void svHandleF12PutMacroMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;

  if (!con)
    return;

  gint res = -1;

  // if there's an existing f12 macro, confirm overwriting first
  if (con->f12Macro->len > 0)
  {
    GtkWidget * dialog = gtk_message_dialog_new(GTK_WINDOW(app->mainWin),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO,
                                    "Are you sure you want to replace the existing F12 macro for '%s'?",
                                    con->name->str);
    res = gtk_dialog_run(GTK_DIALOG(dialog));

    // destroy the dialog box
    gtk_widget_destroy(dialog);

    gtk_window_present(GTK_WINDOW(app->mainWin));
  }

  // only set f12 macro if user confirms 'ok' to overwriting or there's nothing to overwrite
  if (res == GTK_RESPONSE_YES || con->f12Macro->len == 0)
  {
    g_string_assign(con->f12Macro, app->f12Storage->str);
    svConfigWrite();
  }
}


/* clean up connection object if deleted */
void svFreeConnObject(Connection * con)
{
  if (!con)
    return;

  if (con->name)
    g_string_free(con->name, true);
  if (con->group)
    g_string_free(con->group, true);
  if (con->address)
    g_string_free(con->address, true);
  if (con->vncPort)
    g_string_free(con->vncPort, true);
  if (con->vncPass)
    g_string_free(con->vncPass, true);
  if (con->vncLoginUser)
    g_string_free(con->vncLoginUser, true);
  if (con->vncLoginPass)
    g_string_free(con->vncLoginPass, true);
  if (con->sshPort)
    g_string_free(con->sshPort, true);
  if (con->sshPrivKeyfile)
    g_string_free(con->sshPrivKeyfile, true);
  if (con->sshUser)
    g_string_free(con->sshUser, true);
  if (con->sshPass)
    g_string_free(con->sshPass, true);
  if (con->f12Macro)
    g_string_free(con->f12Macro, true);
  if (con->quickNote)
    g_string_free(con->quickNote, true);
  if (con->customCmd1Label)
    g_string_free(con->customCmd1Label, true);
  if (con->customCmd1)
    g_string_free(con->customCmd1, true);
  if (con->customCmd2Label)
    g_string_free(con->customCmd2Label, true);
  if (con->customCmd2)
    g_string_free(con->customCmd2, true);
  if (con->customCmd3Label)
    g_string_free(con->customCmd3Label, true);
  if (con->customCmd3)
    g_string_free(con->customCmd3, true);
  if (con->lastErrorMessage)
    g_string_free(con->lastErrorMessage, true);
  if (con->lastConnectTime)
    g_string_free(con->lastConnectTime, true);
  if (con->clipboard)
    g_string_free(con->clipboard, true);

  g_free(con);
}


/* menu item handler - delete a connection */
void svHandleDeleteMenuItem (GtkMenuItem * gMenuItem, gpointer userData)
{
  Connection * con = (Connection *)userData;

  if (!con)
    return;

  gint res = -1;

  if (con->type != SV_TYPE_VNC_REVERSE)
  {
    GtkWidget * dialog = gtk_message_dialog_new(GTK_WINDOW(app->mainWin),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_QUESTION,
                                    GTK_BUTTONS_YES_NO,
                                    "Are you sure you want to delete '%s'?",
                                    con->name->str);
    res = gtk_dialog_run(GTK_DIALOG(dialog));

    // destroy the dialog box
    gtk_widget_destroy(dialog);

    gtk_window_present(GTK_WINDOW(app->mainWin));
  }

  if (res == GTK_RESPONSE_YES || con->type == SV_TYPE_VNC_REVERSE)
  {
    // get all connection listbox rows
    GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

    // loop through each row
    for (GList * l = rows; l != NULL; l = l->next)
    {
      GtkWidget * row = GTK_WIDGET(l->data);

      // get box that holds status image and label
      GtkWidget * rowBox = gtk_bin_get_child(GTK_BIN(row));

      if (!rowBox)
        continue;

      // get box's children
      GList * boxChildren = gtk_container_get_children(GTK_CONTAINER(rowBox));

      GtkWidget * label = (GtkWidget *)g_list_nth_data(boxChildren, 1);

      g_list_free(boxChildren);

      if (label && GTK_IS_LABEL(label))
      {
        const char * text = gtk_label_get_text(GTK_LABEL(label));

        // found matching row string
        if (strcmp(text, con->name->str) == 0)
        {
          // destroy the child widget (will remove from container too)
          if (row)
            gtk_widget_destroy(row);

          // release and clean up connection resources
          if (app->selectedConnection == con)
            app->selectedConnection = NULL;

          svFreeConnObject(con);

          break;
        }
      }
    }

    // Free the list
    g_list_free(rows);
  }
}


/* menu item handler - add new connection */
void svHandleAddNewConnectionMenuItem (GtkMenuItem * unused1, gpointer unused2)
{
  app->addNewConnection = true;

  svCheckForNewConnectionAdd();
}


/* menu item handler - toggle fullscreen */
void svHandleFullscreenMenuItem (GtkMenuItem * unused1, gpointer unused2)
{
  // if fullscreen, unfullscreen and vice-versa
  if (app->fullscreen)
    gtk_window_unfullscreen(GTK_WINDOW(app->mainWin));
  else
    gtk_window_fullscreen(GTK_WINDOW(app->mainWin));
}


/* check for connected connections */
gboolean svThereAreConnectedConnections ()
{
  gboolean result = false;

  // go through connection listbox and return correct connection
  GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

  for (GList * l = rows; l; l = l->next)
  {
    GtkListBoxRow * row = l->data;
    GtkWidget * box = gtk_bin_get_child(GTK_BIN(row));

    // if there's a connected connection, return true
    Connection * con = g_object_get_data(G_OBJECT(box), "con");
    if (con && con->state == SV_STATE_CONNECTED)
    {
      result = true;
      break;
    }
  }

  g_list_free(rows);

  return result;
}


/* scanMode - move to next connected connection on server list */
gboolean svScanModeNextConnection (gpointer unused)
{
  // if there are no more connected connections, cancel scan mode
  if (!svThereAreConnectedConnections())
  {
    // cancel scan mode
    svCancelScanMode(false);

    // tell user
    svShowMessageDialog("There are no connected connections.\n\nScan mode terminated.");

    gtk_window_present(GTK_WINDOW(app->mainWin));

    return G_SOURCE_REMOVE;
  }

  // see if there's a selected connection
  gint selectedIndex = -1;
  gboolean looped = false;

  GtkListBoxRow * selectedRow = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->serverList));

  // set selectedIndex if there is a selected row
  if (selectedRow)
    selectedIndex = gtk_list_box_row_get_index(selectedRow);

  // go through connection listbox and select the next connected connection
  GList * rows = gtk_container_get_children(GTK_CONTAINER(app->serverList));

  for (GList * l = rows; l; l = l->next)
  {
    GtkListBoxRow * row = l->data;
    gint rowIndex = gtk_list_box_row_get_index(row);;
    GtkWidget * box = gtk_bin_get_child(GTK_BIN(row));

    // if this is a connected connection, process it
    Connection * con = g_object_get_data(G_OBJECT(box), "con");
    if (con && con->state == SV_STATE_CONNECTED)
    {
      // if nothing is selected, select the first connected connection
      // in the list, otherwise select the next connected connection
      if ((selectedIndex == -1) || (rowIndex > selectedIndex))
      {
        gtk_list_box_select_row(GTK_LIST_BOX(app->serverList), GTK_LIST_BOX_ROW(row));
        svConnectionSwitch(con);
        break;
      }
    }

    // we've reached the end of the list, loop around
    if (!l->next)
    {
      // get out if we've already looped, to prevent infinity loopage
      if (looped)
        break;

      looped = true;

      // reset selectedIndex so the first connected connection will be selected / switched to
      selectedIndex = -1;

      // set our iterator to the first row glist element
      l = g_list_first(rows);

      // if for some reason the first element is null, get out
      if (!l)
        break;
    }
  }

  g_list_free(rows);

  return G_SOURCE_CONTINUE;
}


void svCancelScanMode (gboolean removeSource)
{
  // cancel scan mode
  app->scanMode = false;

  // change listen mode tools menu item text
  gtk_menu_item_set_label(GTK_MENU_ITEM(app->toolsItems->scanMode), "Enable sca_n mode");

  // change listen toolbutton to 'enabled' image
  if (app->listenImage)
  {
    gtk_image_clear(GTK_IMAGE(app->scanImage));
    gtk_image_set_from_resource(GTK_IMAGE(app->scanImage), "/com/spiritvnc/pngs/scan-disabled.png");
  }

  // remove timer source
  if (app->scanTimerSource > 0)
  {
    // only remove source if manually canceling scan mode
    if (removeSource)
      g_source_remove(app->scanTimerSource);

    app->scanTimerSource = 0;
  }
}

/* menu item handler - toggle scan mode */
void svHandleScanModeMenuItem (GtkMenuItem * unused1, gpointer unused2)
{
  // toggle scan mode
  app->scanMode = !app->scanMode;

  if (app->scanMode)
  {
    // start scan mode if there's any connected connections
    if (svThereAreConnectedConnections())
    {
      // change listen mode tools menu item text
      gtk_menu_item_set_label(GTK_MENU_ITEM(app->toolsItems->scanMode), "Disable sca_n mode");

      // change listen toolbutton to 'enabled' image
      if (app->listenImage)
      {
        gtk_image_clear(GTK_IMAGE(app->scanImage));
        gtk_image_set_from_resource(GTK_IMAGE(app->scanImage), "/com/spiritvnc/pngs/scan-enabled.png");
      }

      // start scan timer
      app->scanTimerSource = g_timeout_add_seconds(app->scanTimeout, svScanModeNextConnection, NULL);
    }
    else
    {
      // let user know there's no connected connections
      svShowMessageDialog("There are no connected connections.\n\nScan mode not started.");

      gtk_window_present(GTK_WINDOW(app->mainWin));

      // cancel scan mode
      svCancelScanMode(true);
    }
  }
  else
    // cancel scan mode
    svCancelScanMode(true);
}

/* menu item handler - do screenshot of current vnc connection */
void svHandleScreenshotMenuItem (GtkMenuItem * unused1, gpointer unused2)
{
  Connection * con = svGetSelectedConnectionListConnection();
  if (!con)
    return;

  // save screenshot to pixbuf
  GdkPixbuf * pic = vnc_display_get_pixbuf(VNC_DISPLAY(con->vncObj));

  // open save dialog
  GtkWidget * dialog;
  GtkFileChooser * chooser;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
  gint res;

  // get the current time
  GDateTime * now = g_date_time_new_now_local();

  // format the time as a human-readable string
  char * formattedTime = g_date_time_format(now, "%Y-%m-%d--%H-%M-%S");  // <<<--- do NOT make const char *

  GString * existingFilename = g_string_new(NULL);
  g_string_printf(existingFilename, "spiritvnc-screenshot-%s.png", formattedTime);

  // free resources
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

  gtk_file_chooser_set_do_overwrite_confirmation(chooser, true);
  gtk_file_chooser_set_current_name(chooser, existingFilename->str);
  g_string_free(existingFilename, true);

  res = gtk_dialog_run(GTK_DIALOG(dialog));

  if (res == GTK_RESPONSE_ACCEPT)
  {
    char * filename = gtk_file_chooser_get_filename(chooser);  //  <<<--- do NOT make const char *

    gdk_pixbuf_save(pic, filename, "png", NULL, "tEXt::Generator App", "spiritvncgtk", NULL);

    g_free(filename);
    g_object_unref(pic);
  }

  // destroy dialog
  gtk_widget_destroy(dialog);

  gtk_window_present(GTK_WINDOW(app->mainWin));
}


/* create connection right-click menu */
void svConnectionRightClick (Connection * con)
{
  if (!con)
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

  // f12 macro section
  gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), gtk_separator_menu_item_new());

  // get f12 macro menu item
  GtkWidget * getF12 = gtk_menu_item_new_with_label("Get F12 macro");
  svSetTooltip(getF12, "Gets this connection's F12 macro so it can put into another connection's F12 macro");
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(getF12), false);
  gtk_widget_set_sensitive(GTK_WIDGET(getF12), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), getF12);
  g_signal_connect(getF12, "activate", G_CALLBACK(svHandleF12GetMacroMenuItem), con);

  // only enable if this connection has f12 macro text
  if (con->f12Macro->len > 0)
    gtk_widget_set_sensitive(GTK_WIDGET(getF12), true);

  // put f12 macro menu item
  GtkWidget * putF12 = gtk_menu_item_new_with_label("Put F12 macro");
  svSetTooltip(putF12, "Puts a stored F12 macro into this connection's F12 macro");
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(putF12), true);
  gtk_widget_set_sensitive(GTK_WIDGET(putF12), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), putF12);
  g_signal_connect(putF12, "activate", G_CALLBACK(svHandleF12PutMacroMenuItem), con);

  // only enable when connected and there's something to 'put'
  //if (con->state == SV_STATE_CONNECTED && app->f12Storage->len > 0)
  if (app->f12Storage->len > 0)
    gtk_widget_set_sensitive(GTK_WIDGET(putF12), true);

  // delete section
  gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), gtk_separator_menu_item_new());

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

  gboolean enableCustomCommands = false;

  // only display custom commands submenu if any subcommands are enabled and have a label
  if ((con->customCmd1Enabled && con->customCmd1Label->len > 0) ||
    (con->customCmd2Enabled && con->customCmd2Label->len > 0) ||
    (con->customCmd3Enabled && con->customCmd3Label->len > 0))
  {
    enableCustomCommands = true;
  }
  // custom commands submenu
  gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), gtk_separator_menu_item_new());

  // custom commands
  GtkWidget * cc = gtk_menu_item_new_with_label("Custom commands");
  svSetTooltip(cc, "Custom commands sub-menu");
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cc), true);
  gtk_widget_set_sensitive(GTK_WIDGET(cc), enableCustomCommands); //true);
  gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), cc);

  // create submenu
  GtkWidget * customCommands = gtk_menu_new();

  // attach submenu to custom commands item
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(cc), customCommands);

  if (con->customCmd1Enabled && enableCustomCommands)
  {
    // custom command 1
    GtkWidget * cmdLbl1 = gtk_menu_item_new_with_label(con->customCmd1Label->str);
    svSetTooltip(cmdLbl1, "Runs custom command 1");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cmdLbl1), true);
    gtk_widget_set_sensitive(GTK_WIDGET(cmdLbl1), con->customCmd1Enabled);
    gtk_menu_shell_append(GTK_MENU_SHELL(customCommands), cmdLbl1);
    g_signal_connect(cmdLbl1, "activate", G_CALLBACK(svHandleCustomCommandMenuItem), con->customCmd1->str);
  }

  if (con->customCmd2Enabled && enableCustomCommands)
  {
    // custom command 2
    GtkWidget * cmdLbl2 = gtk_menu_item_new_with_label(con->customCmd2Label->str);
    svSetTooltip(cmdLbl2, "Runs custom command 2");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cmdLbl2), true);
    gtk_widget_set_sensitive(GTK_WIDGET(cmdLbl2), con->customCmd2Enabled);
    gtk_menu_shell_append(GTK_MENU_SHELL(customCommands), cmdLbl2);
    g_signal_connect(cmdLbl2, "activate", G_CALLBACK(svHandleCustomCommandMenuItem), con->customCmd2->str);
  }

  if (con->customCmd3Enabled && enableCustomCommands)
  {
    // custom command 3
    GtkWidget * cmdLbl3 = gtk_menu_item_new_with_label(con->customCmd3Label->str);
    svSetTooltip(cmdLbl3, "Runs custom command 3");
    gtk_menu_item_set_use_underline(GTK_MENU_ITEM(cmdLbl3), true);
    gtk_widget_set_sensitive(GTK_WIDGET(cmdLbl3), con->customCmd3Enabled);
    gtk_menu_shell_append(GTK_MENU_SHELL(customCommands), cmdLbl3);
    g_signal_connect(cmdLbl3, "activate", G_CALLBACK(svHandleCustomCommandMenuItem), con->customCmd3->str);
  }
  //}

  // and all the rest, all on gilligan's island
  gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), gtk_separator_menu_item_new());

  // sync clipboard item
  GtkWidget * setClipboard = gtk_menu_item_new_with_label("Set my clipboard to this connection's clipboard");
  svSetTooltip(setClipboard, "Sets this computer's clipboard to the contents of this connection's clipboard");
  gtk_menu_item_set_use_underline(GTK_MENU_ITEM(setClipboard), true);
  if (con->clipboard->len > 0)
    gtk_widget_set_sensitive(GTK_WIDGET(setClipboard), true);
  else
    gtk_widget_set_sensitive(GTK_WIDGET(setClipboard), false);
  gtk_menu_shell_append(GTK_MENU_SHELL(rightMenu), setClipboard);
  g_signal_connect(setClipboard, "activate", G_CALLBACK(svHandleSyncClipboardMenuItem), con);


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

  if (!con)
    return NULL;

  // wait for vnc connection timeout
  for (guint i = 0; i < app->vncConnectWaitTime; i++)
  {
    // break out if connection becomes initialized
    if (con->state != SV_STATE_WAITING)
      break;

    sleep(1);
  }

  // if the connection isn't actually initialized (fully connected), close it
  if (con->state != SV_STATE_CONNECTED)
  {
    if (con->vncObj)
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

  if (!con)
    return;

  // log
  GString * logStr = g_string_new(NULL);
  g_string_printf(logStr, "Server connected (but not init'd) to '%s - %s'", con->name->str, con->address->str);
  svLog(logStr->str, true);
  g_string_free(logStr, true);

  // spawn initialize waiter thread
  GThread * initWaiter G_GNUC_UNUSED = g_thread_new("init-waiter", svConnectionInitWaiter, con);
}


/* handle vnc obj disconnection event */
void svServerDisconnected (GtkWidget * vncObj)
{
  Connection * con = svConnectionFromVNCObj(vncObj);

  if (!con)
    return;


  // log
  GString * logStr = g_string_new(NULL);
  g_string_printf(logStr, "Server disconnected '%s - %s'", con->name->str, con->address->str);
  svLog(logStr->str, true);
  g_string_free(logStr, true);

  // if this is svnc, spawn ssh connection stop thread
  if (con->type == SV_TYPE_VNC_OVER_SSH)
    con->sshCloseThread = g_thread_new("ssh-closer", svSSHConnectionCloser, con);

  // hide the vnc display widget
  gtk_widget_set_visible(con->vncObj, false);

  // remove the vnc display widget from the display stack
  gtk_container_remove(GTK_CONTAINER(app->displayStack), con->vncObj);

  con->vncObj = NULL;

  // * set disconnect state and icon *

  gboolean manualDisconnect = false;

  // manual disconnect
  if (con->state == SV_STATE_CONNECTED && con->disconnectType != SV_DISC_VNC_ERROR && con->disconnectType != SV_DISC_SSH_ERROR)
  {
    manualDisconnect = true;
    con->state = SV_STATE_DISCONNECTED;
  }

  // timeout disconnect
  //else if (con->state == SV_STATE_WAITING && con->state != SV_STATE_TIMEOUT && con->disconnectType != SV_DISC_VNC_ERROR &&
    //con->disconnectType != SV_DISC_SSH_ERROR)
    //con->state = SV_STATE_TIMEOUT;
  else if (con->state == SV_STATE_WAITING && con->disconnectType != SV_DISC_VNC_ERROR &&
    con->disconnectType != SV_DISC_SSH_ERROR)
    con->state = SV_STATE_TIMEOUT;

  // error
  else if (con->state == SV_STATE_ERROR || con->disconnectType == SV_DISC_VNC_ERROR || con->disconnectType == SV_DISC_SSH_ERROR)
    con->state = SV_STATE_ERROR;

  // set icon
  svSetIconFromConnectionName(con->name->str, con->state);

  // automatically delete listening connections on manual disconnect
  if (con->type == SV_TYPE_VNC_REVERSE && manualDisconnect)
    svHandleDeleteMenuItem(NULL, con);
}


/* handle vnc obj connection error event */
void svServerError (VncConnection * conn, const char * message, void * data)
{
  if (!message)
    return;

  Connection * con = (Connection *)data;

  if (!con)
    return;

  // log
  GString * logStr = g_string_new(NULL);
  g_string_printf(logStr, "Server error '%s - %s': %s", con->name->str, con->address->str, message);
  svLog(logStr->str, true);
  g_string_free(logStr, true);

  // set state based on 'timeout' or 'timed out' string in error message
  if (strstr(message, "timeout") || strstr(message, "timed out"))
    con->state = SV_STATE_TIMEOUT;
  else
    con->state = SV_STATE_ERROR;

  svSetIconFromConnectionName(con->name->str, con->state);

  // set connection's last error message
  g_string_assign(con->lastErrorMessage, message);

  // only show newly-connected server if the listitem is selected
  const char * selectedRowText = svSelectedRowText();

  if (selectedRowText && strcmp(con->name->str, selectedRowText) == 0)
    gtk_text_buffer_set_text(app->quickNoteLastErrorBuffer, con->lastErrorMessage->str, -1);
}


/* handle vnc obj initialize event (fully connected) */
void svServerInitialized (GtkWidget * vncObj)
{
  Connection * con = svConnectionFromVNCObj(vncObj);

  if (!con)
    return;

  // log
  GString * logStr = g_string_new(NULL);
  g_string_printf(logStr, "Server initialized (fully connected) '%s - %s'", con->name->str, con->address->str);
  svLog(logStr->str, true);
  g_string_free(logStr, true);

  con->state = SV_STATE_CONNECTED;
  svSetIconFromConnectionName(con->name->str, con->state);

  // add the vnc obj to the display stack
  gtk_stack_add_named(GTK_STACK(app->displayStack), vncObj, con->name->str);

  // set last connected time
  GDateTime * now = g_date_time_new_now_local();

  char * nowStr = g_date_time_format(now, "%H:%M:%S--%Y-%m-%d");  // <<<--- do NOT make const char *
  g_string_assign(con->lastConnectTime, nowStr);
  g_free(nowStr);

  // change listening display text and name to actual remote name, if available
  if (con->type == SV_TYPE_VNC_REVERSE)
  {
    const char * remoteName = vnc_display_get_name(VNC_DISPLAY(vncObj));

    if (remoteName && remoteName[0] != '\0')
    {
        // make time string
        char * nowStrListen = g_date_time_format(now, "-%Y%m%d%H%M%S");  //  <<<--- do NOT make const char *
        GString * fullRemoteName = g_string_new(remoteName);
        g_string_append(fullRemoteName, nowStrListen);
        g_free(nowStrListen);

      svSetTextFromConnectionName(con->name->str, fullRemoteName->str);
      g_string_assign(con->name, fullRemoteName->str);
      g_string_free(fullRemoteName, true);
    }
  }

  g_date_time_unref(now);

  // update tooltips info
  svSetHostlistItemsTooltips();

  // only show newly-connected server if the listitem is selected
  const char * selectedRowText = svSelectedRowText();

  if (selectedRowText && strcmp(con->name->str, selectedRowText) == 0)
  {
    // set 'last connected' label text
    if (con->lastConnectTime->len > 0)
    {
      GString * strTime = g_string_new(NULL);
      g_string_printf(strTime, "Last connected: %s", con->lastConnectTime->str);
      gtk_label_set_text(GTK_LABEL(app->quickNoteLastConnected), strTime->str);
      g_string_free(strTime, true);
    }
    else
      gtk_label_set_text(GTK_LABEL(app->quickNoteLastConnected), "-");

    // show the remote server screen
    gtk_widget_show(vncObj);
    gtk_stack_set_visible_child(GTK_STACK(app->displayStack), vncObj);

    // set keyboard focus
    gtk_widget_set_can_focus(vncObj, true);
    // tell the parent container that THIS is the focus child
    gtk_container_set_focus_child(GTK_CONTAINER(app->displayStackScroller), app->displayStack);
    gtk_container_set_focus_child(GTK_CONTAINER(app->displayStack), vncObj);

    /* Attempt to focus the vnc display. Unfortunately we have to get a little 'creative'
     * here and try two different ways (because the idle_add doesn't always work)
     */

    // try to focus the vnc display at idle time
    g_idle_add((GSourceFunc)svFocusOnce, vncObj);
    // try to focus the vnc display (again) after 300 milliseconds
    g_timeout_add_once(300, (GSourceOnceFunc)gtk_widget_grab_focus, vncObj);

    gtk_widget_grab_focus(vncObj);

    // set tools menu items
    svSetToolsMenuItems(true);
  }
}


/* handle vnc obj authentication event */
void svServerAuthenticate (VncDisplay * display, GValueArray * credList)
{
  const char ** data = g_new0(const char *, credList->n_values);
  gsize outLen = 0;
  gboolean hasUsername = false;

  Connection * con = svConnectionFromVNCObj(GTK_WIDGET(display));

  // if connection is null, close the connection and get out
  if (!con)
  {
    vnc_display_close(display);
    svLog("svServerAuthenticate - ERROR: con is null", false);
    return;
  }

  // log
  GString * logStr = g_string_new(NULL);
  g_string_printf(logStr, "Server authentication '%s - %s'", con->name->str, con->address->str);
  svLog(logStr->str, true);
  //g_string_free(logStr, true);  // <<<--- NO, is used below!

  // set vnc login username, if any
  const char * loginUser = con->vncLoginUser->str;

  // decode vnc and login passwords, if any
  unsigned char * vncPass = g_base64_decode(con->vncPass->str, &outLen);
  unsigned char * loginPass = g_base64_decode(con->vncLoginPass->str, &outLen);

  // loop through requested credentials and set each
  for (guint i = 0 ; i < credList->n_values ; i++)
  {
    GValue * cred = credList->values + i; //1;

    switch (g_value_get_enum(cred))
    {
      case VNC_DISPLAY_CREDENTIAL_USERNAME:
        hasUsername = true;
        data[i] = (char *)loginUser;
        svLog("Credential 'Username'", true);
        break;
      case VNC_DISPLAY_CREDENTIAL_PASSWORD:
      {
        if (hasUsername)
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
  for (guint i = 0 ; i < credList->n_values ; i++)
  {
    GValue * cred = credList->values + i; //1;

    if (data[i])
    {
      if (vnc_display_set_credential(VNC_DISPLAY(display), g_value_get_enum(cred), data[i]))
      {
        // log
        g_string_printf(logStr, "Failed to set credential type '%d'", g_value_get_enum(cred));
        svLog(logStr->str, false);

        vnc_display_close(VNC_DISPLAY(display));
        con->state = SV_STATE_DISCONNECTED;
      }
    }
    else
    {
      // log
      g_string_printf(logStr, "Unsupported credential type '%d'", g_value_get_enum(cred));
      svLog(logStr->str, false);

      vnc_display_close(VNC_DISPLAY(display));
      con->state = SV_STATE_DISCONNECTED;
    }
  }

  g_string_free(logStr, true);
  g_free(vncPass);
  g_free(loginPass);
  g_free(data);
}


/* saves the last selected connection's quicknote text */
/* (usually before switching to another connection) */
void svSavePreviousQuickNoteText (const Connection * newConnection)
{
  // save previous quicknote before switching
  Connection * prevCon = app->selectedConnection;

  if (!prevCon || prevCon == newConnection)
    return;

  if (!prevCon->name || prevCon->name->len == 0)
    return;

  // quickNoteBuffer must exist
  if (!app->quickNoteBuffer)
    return;

  // prevCon->quickNote must exist
  if (!prevCon->quickNote)
    return;

  GtkTextIter start;
  GtkTextIter end;

  gtk_text_buffer_get_start_iter(app->quickNoteBuffer, &start);
  gtk_text_buffer_get_end_iter(app->quickNoteBuffer, &end);

  char * qText = gtk_text_buffer_get_text(app->quickNoteBuffer, &start, &end, false);
  if (!qText)
    return;

  char * encoded = g_base64_encode((const unsigned char *)qText, strlen(qText));
  g_string_assign(prevCon->quickNote, encoded);

  g_free(encoded);
  g_free(qText);
}


/* handle vnc obj 'server cut / copy (clipboard) text' event */
void svHandleServerClipboard (VncConnection * unused, const char * text, void * data)
{
  //printf("handle server clipboard\n");

  if (!data || !text)
    return;

  if (strlen(text) < 1)
    return;

  Connection * con = (Connection *)data;

  // store this server's clipboard text to this connection's clipboard string
  g_string_assign(con->clipboard, text);
}


/* handle key presses */
/* NOTE: signal only fires from active VncDisplay */
gboolean svHandleKeyboard (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  // get the selected connection
  Connection * con = svGetSelectedConnectionListConnection();

  // f8 - connection actions
  if (event->keyval == GDK_KEY_F8)
  {
    if (!con || con->name->len == 0 || con->state != SV_STATE_CONNECTED)
      return false;

    svShowConnectionActionsWindow();
    return true;
  }

  // f10 - screenshot
  if (event->keyval == GDK_KEY_F9)
  {
    if (!con || con->name->len == 0 || con->state != SV_STATE_CONNECTED)
      return false;

    svHandleScreenshotMenuItem(NULL, NULL);
    return true;
  }

  // f11 - hide host list
  if (event->keyval == GDK_KEY_F11)
  {
    // get current list width
    gint listWidth = gtk_paned_get_position(GTK_PANED(app->parent));

    if (listWidth < 1)
      // restore if hidden
      gtk_paned_set_position(GTK_PANED(app->parent), app->serverListWidthLast);
    else
    {
      // store current hostlist width
      app->serverListWidthLast = listWidth;

      // hide list
      gtk_paned_set_position(GTK_PANED(app->parent), 0);
    }

    return true;
  }

  // f12 macro
  if (event->keyval == GDK_KEY_F12)
  {
    if (!con || con->name->len == 0)
      return false;

    SendKeysObj * obj = g_new0(SendKeysObj, 1);
    if (!obj)
      return false;

    // set the obj properties for 'OTHER' type
    obj->type = SV_SENDKEYS_TYPE_OTHER;
    obj->win = NULL;
    obj->con = con;
    obj->textToSend = g_string_new(con->f12Macro->str);

    // send the f12 macro
    svHandleSendEnteredKeystrokesSend(NULL, obj);

    return true;  // stop further handling
  }

  return false;   // allow normal processing
}


/* create and start a new connection */
void svConnectionCreate (Connection * con)
{
  //  get out if state is CONNECTED or WAITING
  if (!con || con->state == SV_STATE_CONNECTED || con->state == SV_STATE_WAITING)
    return;

  GString * logStr = g_string_new(NULL);

  // log
  if (con->type != SV_TYPE_VNC_REVERSE)
    g_string_printf(logStr, "Creating new connection for '%s - %s'", con->name->str, con->address->str);
  else
    g_string_printf(logStr, "Creating new listening connection");

  // log
  svLog(logStr->str, true);
  g_string_free(logStr, true);

  // create a new vnc obj
  GtkWidget * vnc = vnc_display_new();

  if (!vnc)
  {
    svLog("svConnectionCreate - CRITICAL: Unable to create a new vnc object!", false);
    return;
  }

  // clear error quicknote text
  gtk_text_buffer_set_text(app->quickNoteLastErrorBuffer, "", -1);

  // set this connection's vnc obj
  con->vncObj = vnc;

  // connect vnc obj signals
  g_signal_connect(con->vncObj, "vnc-initialized", G_CALLBACK(svServerInitialized), NULL);
  g_signal_connect(con->vncObj, "vnc-connected", G_CALLBACK(svServerConnected), NULL);
  g_signal_connect(con->vncObj, "vnc-disconnected", G_CALLBACK(svServerDisconnected), NULL);
  g_signal_connect(con->vncObj, "vnc-auth-credential", G_CALLBACK(svServerAuthenticate), NULL);
  g_signal_connect(con->vncObj, "vnc-error", G_CALLBACK(svServerError), con);
  g_signal_connect(con->vncObj, "vnc-server-cut-text", G_CALLBACK(svHandleServerClipboard), con);
  g_signal_connect(con->vncObj, "key-press-event", G_CALLBACK(svHandleKeyboard), con);

  // change connection list icon
  svSetIconFromConnectionName(con->name->str, SV_STATE_WAITING);

  // reset connection variables
  con->state = SV_STATE_WAITING;
  con->disconnectType = SV_DISC_NONE;
  g_string_truncate(con->lastErrorMessage, 0);

  // process based on type
  switch (con->type)
  {
    case SV_TYPE_VNC:
    case SV_TYPE_VNC_REVERSE:
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


/* set error message on ssh failure */
void svSetSSHLastErrorMessage (gpointer data)
{
  Connection * con = (Connection *)data;
  if (!con)
    return;

  // get the buffer's end iter
  GtkTextIter end;
  gtk_text_buffer_get_end_iter(app->quickNoteLastErrorBuffer, &end);

  // build new string
  GString * newText = g_string_new(NULL);

  // set the string to the latest error message
  g_string_printf(newText, "%s\n- -\n", con->lastErrorMessage->str);

  // insert the new error text at the end of the quicknote error area
  gtk_text_buffer_insert(app->quickNoteLastErrorBuffer, &end, newText->str, -1);

  g_string_free(newText, true);
}


/* set error icon from ssh error */
void svSetConnectionIconFromSSHError (gpointer data)
{
  Connection * con = (Connection *)data;
  if (!con)
    return;

  svSetIconFromConnectionName(con->name->str, SV_STATE_ERROR);
}


/* monitor ssh connection state and take appropriate action */
gpointer svSSHMonitor (gpointer data)
{
  Connection * con = (Connection *)data;

  if (!con)
    return NULL;

  // wait for ssh connection
  for (guint i = 0; i < app->sshConnectWaitTime; i++)
  {
    if (con->sshContinue)
      break;

    sleep(1);
  }

  // deal with ssh connection timeout / failure
  if (!con->sshContinue)
  {
    // set connection's last error message
    g_string_assign(con->lastErrorMessage, "Could not connect to SSH server");
    svLog(con->lastErrorMessage->str, false);

    // set last error message text view
    g_idle_add_once(svSetSSHLastErrorMessage, con);

    // set connection variables
    con->sshContinue = false;
    con->disconnectType = SV_DISC_SSH_ERROR;
    con->state = SV_STATE_ERROR;

    // set connection list row's icon
    g_idle_add_once(svSetConnectionIconFromSSHError, con);

    return NULL;
  }

  // if all seems well, wait to show
  for (guint i = 0; i < app->sshShowWaitTime; i++)
    sleep(1);

  // log
  GString * logStr = g_string_new(NULL);
  g_string_printf(logStr, "Attempting SSH connection open '%s - %s'", con->name->str, con->address->str);
  svLog(logStr->str, true);
  g_string_free(logStr, true);

  // attempt to connect to forwarded vnc
  svConnectionOpen(con);

  return NULL;
}


/* attempts to write closing commands to the ssh thread's stream */
gpointer svSSHConnectionCloser (gpointer data)
{
  Connection * con = (Connection *)data;

  if (!con || !con->sshStream)
    return NULL;

  // log
  GString * logStr = g_string_new(NULL);
  g_string_printf(logStr, "Attempting to close SSH connection '%s - %s'", con->name->str, con->address->str);
  svLog(logStr->str, true);
  g_string_free(logStr, true);

  // send 'exit' and 'exit' control-char sequence
  fputs("\r\n", con->sshStream);
  fputs("exit\n\r\n", con->sshStream);
  fputs("~.\r\n", con->sshStream);
  fflush(con->sshStream);

  // close the ssh process stream
  pclose(con->sshStream);

  con->sshContinue = false;

  return NULL;
}


/* ask connection to end */
/* (ssh cleanup is performed in svServerDisconnected) */
void svConnectionEnd (Connection * con)
{
  if (!con)
    return;

  // log
  GString * logStr = g_string_new(NULL);

  if (con->type != SV_TYPE_VNC_REVERSE)
    g_string_printf(logStr, "Ending connection '%s - %s'", con->name->str, con->address->str);
  else
    g_string_printf(logStr, "Ending listening connection '%s'", con->name->str);

  svLog(logStr->str, true);
  g_string_free(logStr, true);

  // close the vnc display connection
  if (con->vncObj)
    vnc_display_close(VNC_DISPLAY(con->vncObj));

  // set tools menu items
  svSetToolsMenuItems(false);

  // clear connection clipboard
  g_string_truncate(con->clipboard, 0);
}


/* set tools menu connection-related items state */
void svSetToolsMenuItems(gboolean sState)
{
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->requestUpdate), sState);
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->send), sState);
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->sendEnteredKeys), sState);
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->sendCAD), sState);
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->sendCSE), sState);
  gtk_widget_set_sensitive(GTK_WIDGET(app->toolsItems->screenshot), sState);
}


gboolean svFocusOnce (gpointer data)
{
  GtkWidget * w = GTK_WIDGET(data);

  if (GTK_IS_WIDGET(w))
    gtk_widget_grab_focus(w);

  return FALSE;  // run once, then stop
}

/* switch from one connection to another */
/* (usually from a single click on the connection listbox) */
void svConnectionSwitch (Connection * con)
{
  // ***### DO NOT CHECK CON FOR NULL HERE!!! ###***

  // no re-entry
  static gboolean inConnectionSwitch = false;

  if (inConnectionSwitch)
    return;

  inConnectionSwitch = true;

  // disable tools menu items initially
  svSetToolsMenuItems(false);

  // save previous quicknote before switching
  svSavePreviousQuickNoteText(con);

  // there's no connection here, blank quicknote stuffs
  if (!con)
  {
    app->selectedConnection = NULL;
    gtk_label_set_text(GTK_LABEL(app->quickNoteLabel), "-");
    gtk_label_set_text(GTK_LABEL(app->quickNoteLastConnected), "-");
    gtk_text_buffer_set_text(app->quickNoteLastErrorBuffer, "", -1);
    gtk_text_buffer_set_text(app->quickNoteBuffer, "", -1);

    inConnectionSwitch = false;

    return;
  }
  else
  {
    // log
    GString * logStr = g_string_new(NULL);
    g_string_printf(logStr, "Switching to connection '%s - %s'", con->name->str, con->address->str);
    svLog(logStr->str, true);
    g_string_free(logStr, true);

    // fill quicknote label, last error text and quicknote text
    gtk_label_set_text(GTK_LABEL(app->quickNoteLabel), con->name->str);

    if (con->lastConnectTime->len > 0)
    {
      GString * strTime = g_string_new(NULL);
      g_string_printf(strTime, "Last connected: %s", con->lastConnectTime->str);
      gtk_label_set_text(GTK_LABEL(app->quickNoteLastConnected), strTime->str);
      g_string_free(strTime, true);
    }
    else
      gtk_label_set_text(GTK_LABEL(app->quickNoteLastConnected), "-");

    gtk_text_buffer_set_text(app->quickNoteLastErrorBuffer, con->lastErrorMessage->str, -1);

    gsize outLen = 0;
    unsigned char * qNote = g_base64_decode(con->quickNote->str, &outLen);
    gtk_text_buffer_set_text(app->quickNoteBuffer, (const char *)qNote, -1);
    g_free(qNote);

    // set selected connection to passed connection
    app->selectedConnection = con;

    // show vnc obj if it's connected
    if (con->state == SV_STATE_CONNECTED && con->vncObj)
    {
      // show vnc display
      gtk_widget_set_visible(con->vncObj, true);
      gtk_stack_set_visible_child(GTK_STACK(app->displayStack), con->vncObj);
      // set keyboard focus
      gtk_widget_set_can_focus(con->vncObj, true);
      // tell the parent container that THIS is the focus child
      gtk_container_set_focus_child(GTK_CONTAINER(app->displayStackScroller), app->displayStack);
      gtk_container_set_focus_child(GTK_CONTAINER(app->displayStack), con->vncObj);

      /* Attempt to focus the vnc display. Unfortunately we have to get a little 'creative'
       * here and try two different ways (because the idle_add doesn't always work)
       */

      // try to focus the vnc display at idle time
      g_idle_add((GSourceFunc)svFocusOnce, con->vncObj);
      // try to focus the vnc display (again) after 300 milliseconds
      g_timeout_add_once(300, (GSourceOnceFunc)gtk_widget_grab_focus, con->vncObj);

      gtk_widget_grab_focus(con->vncObj);

      // set tools menu items
      svSetToolsMenuItems(true);
    }
  }

  inConnectionSwitch = false;
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

  // set scaling
  vnc_display_set_scaling(VNC_DISPLAY(con->vncObj), con->scale);
  vnc_display_set_keep_aspect_ratio(VNC_DISPLAY(con->vncObj), TRUE);

  // initiate connection
  switch (con->type)
  {
    case SV_TYPE_VNC:
      // connect vnc obj directly to remote host
      vnc_display_open_host(VNC_DISPLAY(con->vncObj), con->address->str, con->vncPort->str);
      break;

    case SV_TYPE_VNC_REVERSE:
      // connect vnc obj directly to remote host
      vnc_display_open_fd(VNC_DISPLAY(con->vncObj), con->listenFd);
      break;

    case SV_TYPE_VNC_OVER_SSH:
    {
      // set up local port string
      GString * localPortStr = g_string_new(NULL);
      g_string_printf(localPortStr, "%ui", con->sshLocalPort);

      // connect vnc obj to local forwarded port
      vnc_display_open_host(VNC_DISPLAY(con->vncObj), "127.0.0.1", localPortStr->str);
      g_string_free(localPortStr, true);
      break;
    }

    default:
      return;
  }

  // vnc display is added to the stack in 'svServerInitialized' method
}


/* show error message about null con fields */
void svShowNullConFieldsDialog (gpointer unused)
{
  svShowMessageDialog("One more more connection properties were NULL or invalid when attempting a SSH connection");

  gtk_window_present(GTK_WINDOW(app->mainWin));
}


/* show about window */
void svShowAboutWindow ()
{
  GdkPixbuf * aboutImage = gdk_pixbuf_new_from_resource("/com/spiritvnc/pngs/spiritvnc.png", NULL);
  if (!aboutImage)
    return;

  GtkWidget * aboutDialog = gtk_about_dialog_new();
  const char * authors[] = {"Will Brokenbourgh", NULL};

  // set about dialog properties
  gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(aboutDialog), "SpiritVNC-GTK");
  gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(aboutDialog), aboutImage);
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(aboutDialog), SV_APP_VERSION);
  gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(aboutDialog), authors);
  //gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(aboutDialog), "https://www.willbrokenbourgh.com/brainout/");
  //gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(aboutDialog), "Will Brokenbourgh");
  gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(aboutDialog), "SpiritVNC is released under a BSD 3-Clause license");
  gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(aboutDialog), "A multi-connection VNC viewer for Unix-like, macOS and "
    "Windows systems");

  // set window properties
  gtk_window_set_transient_for(GTK_WINDOW(aboutDialog), GTK_WINDOW(app->mainWin));
  gtk_window_set_position(GTK_WINDOW(aboutDialog), GTK_WIN_POS_CENTER_ON_PARENT);

  // show the dialog then destroy afterwards
  gtk_dialog_run(GTK_DIALOG(aboutDialog));
  gtk_widget_destroy(aboutDialog);

  // focus the main window when done
  gtk_window_present(GTK_WINDOW(app->mainWin));
}


/* create ssh session and ssh forwarding */
/* (run as a thread due to blocking) */
gpointer svCreateSSHConnection (gpointer data)
{
  Connection * con = (Connection *)data;

  if (!con)
    return NULL;

  // log
  GString * logStr = g_string_new(NULL);
  g_string_printf(logStr, "Creating new SSH connection for '%s - %s'", con->name->str, con->address->str);
  svLog(logStr->str, true);
  g_string_free(logStr, true);

  #ifdef _WIN32
  // deal with annoying Windows terminal window issues
  AllocConsole();
  ShowWindow(GetConsoleWindow(), SW_HIDE);
  #endif

  // build the ssh check command
  char * sshCheck[] = {
    app->sshCommand->str,
    NULL
  };

  // first check to see if the ssh command is working
  GError * gSSHCheckError = NULL;
  char * stdOut = NULL;
  char * stdErr = NULL;
  gint exitStatus = 0;

  //for (gint i = 0; sshCheck[i] != NULL; i++)
  //{
    //printf("%s\n", sshCheck[i]);
  //}

  gboolean checkResult = g_spawn_sync(NULL,         // Working directory (NULL uses current directory)
                      sshCheck,      // Command to execute
                      NULL,         // Environment variables
                      G_SPAWN_DEFAULT, // Flags
                      NULL,         // Child setup function
                      NULL,         // User data for child setup
                      &stdOut,         // Standard output (NULL ignores it)
                      &stdErr,      // Standard error (NULL ignores it)
                      &exitStatus, // Exit status
                      &gSSHCheckError);

  (void)checkResult;

  // deal with ssh command check failure
  // I'm only interested in gSSHCheckError and I'm purposely ignoring checkResult and exitStatus
  if (gSSHCheckError)
  {
    // problemos
    printf("ssh error - gError code: %i, gError message: %s\n", gSSHCheckError->code, gSSHCheckError->message);
    //printf("ssh error - exitStatus: %i, stdErr: %s\n", exitStatus, stdErr);

    // log error
    GString * errStr = g_string_new(NULL);
    g_string_printf(errStr,
      "SSH command not working for connection '%s - %s'\nError: %s\nCheck command or installation",
      con->name->str, con->address->str, gSSHCheckError->message);
    svLog(errStr->str, false);

    // set connection's last error message
    g_string_assign(con->lastErrorMessage, errStr->str);
    g_idle_add_once(svSetSSHLastErrorMessage, con);
    g_string_free(errStr, true);

    // set connection variables
    con->sshContinue = false;
    con->disconnectType = SV_DISC_SSH_ERROR;
    con->state = SV_STATE_ERROR;

    // set connection list row's icon
    g_idle_add_once(svSetConnectionIconFromSSHError, con);

    g_free(stdOut);
    g_free(stdErr);

    //printf("DEBUG: svCreateSSHConnection - readError pointer = %p\n", (void *)gError);
    g_error_free(gSSHCheckError);

    return NULL;
  }

  g_free(stdOut);
  g_free(stdErr);

  //if (gSSHCheckError)
  //{
    ////printf("DEBUG: svCreateSSHConnection - readError pointer = %p\n", (void *)gSSHCheckError);
    //g_error_free(gSSHCheckError);
  //}

  // --- looks like we're okay ---

  // check that all con fields are non-null
  if (!con->sshUser || !con->address || con->sshPort->len == 0 ||
      con->sshLocalPort == 0 || !con->vncPort || !con->sshPrivKeyfile)
  {
    g_idle_add_once(svShowNullConFieldsDialog, NULL);
    return NULL;
  }

  GError * sshRunErr = NULL;
  GPid pid = 0;

  // ** build the command string for our g_spawn_async_with_pipes call **

  // build "user@host"
  GString * tgt = g_string_new(NULL);
  g_string_printf(tgt, "%s@%s", con->sshUser->str, con->address->str);
  char * sshTargetString = tgt->str;

  // build "localPort:127.0.0.1:vncPort"
  GString * fwd = g_string_new(NULL);
  g_string_printf(fwd, "%i:127.0.0.1:%s", con->sshLocalPort, con->vncPort->str);
  char * localForwardString = fwd->str;

  // build argv array for SSH
  char * sshArgv[] =
  {
    app->sshCommand->str,          // "ssh"
    "-t",
    "-t",
    "-p", con->sshPort->str,
    "-o", "ConnectTimeout=5",
    "-L", localForwardString,
    "-i", con->sshPrivKeyfile->str,
    sshTargetString,
    NULL
  };

  //for (gint i = 0; sshArgv[i] != NULL; i++)
  //{
    //printf("%s\n", sshArgv[i]);
  //}

  // attempt to call the system's ssh client and open write stream
  gboolean sshRunOkay = g_spawn_async_with_pipes(
                  NULL,
                  sshArgv,               // parsed SSH command
                  NULL,
                  G_SPAWN_DO_NOT_REAP_CHILD,
                  NULL,
                  NULL,
                  &pid,
                  &con->sshStdIn,
                  NULL,
                  NULL,
                  &sshRunErr);

  (void)sshRunOkay;

  // we only care about sshRunError, not sshRunOkay
  if (sshRunErr)
  {
    // something -- happened - log the failure
    GString * errStr = g_string_new(NULL);
    g_string_printf(errStr, "SSH command failed for '%s - %s'", con->name->str, con->address->str);
    svLog(errStr->str, false);

    // set connection's last error message
    g_string_assign(con->lastErrorMessage, errStr->str);
    g_idle_add_once(svSetSSHLastErrorMessage, con);
    g_string_free(errStr, true);

    // set connection variables
    con->sshContinue = false;
    con->disconnectType = SV_DISC_SSH_ERROR;
    con->state = SV_STATE_ERROR;

    // set connection list row icon
    g_idle_add_once(svSetConnectionIconFromSSHError, con);

    // free up stuff
    g_string_free(tgt, true);
    g_string_free(fwd, true);

    g_error_free(sshRunErr);

    return NULL;
  }

  // free up stuff
  g_string_free(tgt, true);
  g_string_free(fwd, true);

  con->sshPid = pid;

  // open stdin stream
  con->sshStream = fdopen(con->sshStdIn, "w");

  // stdin stream failed to open
  if (!con->sshStream)
  {
    // something -- happened - log the failure
    GString * errStr = g_string_new(NULL);
    g_string_printf(errStr, "Failed to open SSH input stream for '%s - %s'", con->name->str, con->address->str);
    svLog(errStr->str, false);

    // set connection's last error message
    g_string_assign(con->lastErrorMessage, errStr->str);
    g_idle_add_once(svSetSSHLastErrorMessage, con);
    g_string_free(errStr, true);

    // set connection variables
    con->sshContinue = false;
    con->disconnectType = SV_DISC_SSH_ERROR;
    con->state = SV_STATE_ERROR;

    // set connection list row icon
    g_idle_add_once(svSetConnectionIconFromSSHError, con);

    return NULL;
  }

  con->sshContinue = true;

  return NULL;
}

/* handle mac menu bar 'quit' action */
void svAppMenuAboutAction (GSimpleAction * action, GVariant * parameter, gpointer user_data)
{
  svShowAboutWindow();
}


/* handle mac menu bar 'quit' action */
void svAppMenuSettingsAction (GSimpleAction * action, GVariant * parameter, gpointer user_data)
{
  svShowAppOptionsWindow();
}

/* handle mac menu bar 'quit' action */
void svAppMenuQuitAction (GSimpleAction * action, GVariant * parameter, gpointer user_data)
{
  svDoQuit();
  g_application_quit(G_APPLICATION(user_data));
}


/* handle app's activate event */
static void svAppActivate (GtkApplication * gtkApp, gpointer userData)
{
  svCreateGUI(gtkApp);
  svLog("--- App starting up ---", false);
}


/* main program */
gint main (gint argc, char ** argv)
{
  // give life to our app variable
  app = g_new0(App, 1);

  // initialize the app struct
  svInitAppVars();

  // set application stuffs
  app->gApp = gtk_application_new ("org.will.brokenbourgh", G_APPLICATION_DEFAULT_FLAGS);
  const GActionEntry app_actions[] = {
    {"about", svAppMenuAboutAction, NULL, NULL, NULL},
    {"preferences", svAppMenuSettingsAction, NULL, NULL, NULL},
    {"quit", svAppMenuQuitAction, NULL, NULL, NULL}
  };

  g_action_map_add_action_entries(G_ACTION_MAP(app->gApp), app_actions, G_N_ELEMENTS(app_actions), app->gApp);

  g_signal_connect(app->gApp, "activate", G_CALLBACK(svAppActivate), NULL);

  // run our app
  gint status = g_application_run(G_APPLICATION(app->gApp), argc, argv);

  // unref after we're done
  g_object_unref(app->gApp);

  g_free(app);

  return status;
}
