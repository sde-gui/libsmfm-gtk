#ifndef FM_INTERNALS_TIME_INTERVAL_RENDERER_H
#define FM_INTERNALS_TIME_INTERVAL_RENDERER_H

typedef struct {
    time_t min_time;
    time_t max_time;
} TimeIntervalRenderer;

static void time_interval_renderer_init(TimeIntervalRenderer * self, time_t time)
{
    self->min_time = time;
    self->max_time = time;
}

static void time_interval_renderer_inject(TimeIntervalRenderer * self, time_t time)
{
    if (self->min_time > time)
        self->min_time = time;
    if (self->max_time < time)
        self->max_time = time;
}

static void time_interval_renderer_render(TimeIntervalRenderer * self, GtkLabel * label, const char * format)
{
    if (!label)
        return;

    struct tm tm;
    char buf1[256], buf2[256];

    time_t min_time = self->min_time;
    time_t max_time = self->max_time;

    localtime_r(&min_time, &tm);
    strftime(buf1, sizeof(buf1), format, &tm);

    if (min_time == 0)
    {
        gtk_label_set_text(label, _("N/A"));
    }
    else if (min_time == max_time)
    {
        gtk_label_set_text(label, buf1);
    }
    else
    {
        localtime_r(&max_time, &tm);
        strftime(buf2, sizeof(buf2), format, &tm);

        gchar * s = g_strdup_printf(_("%s ... %s"), buf1, buf2);
        gtk_label_set_text(label, s);
        g_free(s);
    }

}

#endif
