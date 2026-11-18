#ifndef ND_PANEL_H
#define ND_PANEL_H
/* A non-modal native panel: the owner keeps serving dock messages. */
#define ND_PANEL_PERCENT 65535U
struct NDPanel;
/* Percentage sliders emit their field ID only when released. */
int nd_panel_percent_get(struct NDPanel *panel,unsigned int index);
void nd_panel_percent_set(struct NDPanel *panel,unsigned int index,int value,int enabled);
struct NDPanelField {const char *label,*value;unsigned int limit;};
struct NDPanel *nd_panel_open(const char *title,const struct NDPanelField *fields,
                              unsigned int count,const char **buttons,unsigned int buttons_count);
void nd_panel_close(struct NDPanel *panel);
void nd_panel_raise(struct NDPanel *panel);
unsigned long nd_panel_signals(const struct NDPanel *panel);
/* 0 no action, -1 close/Escape, 100+button index, 1+field index on Return/Tab. */
int nd_panel_event(struct NDPanel *panel);
const char *nd_panel_get(struct NDPanel *panel,unsigned int index);
void nd_panel_set_label(struct NDPanel *panel,unsigned int index,const char *label);
void nd_panel_set(struct NDPanel *panel,unsigned int index,const char *text);
void nd_panel_title(struct NDPanel *panel,const char *title);
void nd_panel_sync_string(struct NDPanel *panel,unsigned int index,const char *value);
void nd_panel_commit_string(struct NDPanel *panel,unsigned int index);
void nd_panel_enable(struct NDPanel *panel,unsigned int index,int enabled);
void nd_panel_beep(struct NDPanel *panel);
#endif
