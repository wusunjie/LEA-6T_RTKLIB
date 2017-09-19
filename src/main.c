extern int navi_start(void);
extern int navi_stop(void);
extern int navi_event_handle(void);

int main(void)
{
    navi_start();

    while (1) {
        navi_event_handle();
    }

    navi_stop();

    return 0;
}
