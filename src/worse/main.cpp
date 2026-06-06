import worse.application.window;

int main()
{
    using namespace worse;

    application::Window wd;

    wd.initialize({}, true);

    while (true)
    {
        wd.Poll();
    }

    wd.destory();
}