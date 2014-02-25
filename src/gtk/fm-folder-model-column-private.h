
typedef struct
{
    FmFolderModelCol id;
    GType type;
    const char *name;
    const char *title;
    gboolean sortable;
    gint default_width;
    void (*get_value)(FmFileInfo *fi, GValue *value);
    gint (*compare)(FmFileInfo *fi1, FmFileInfo *fi2);
} FmFolderModelInfo;

extern guint column_infos_n;
extern FmFolderModelInfo ** column_infos;
