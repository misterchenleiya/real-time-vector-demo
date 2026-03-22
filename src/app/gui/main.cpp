#include "app/gui/main_window.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("DEMO"));
    app.setApplicationDisplayName(QStringLiteral("DEMO"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("real-time-vector-demo GUI"));
    parser.addHelpOption();

    QCommandLineOption configOption(
        QStringList() << QStringLiteral("c") << QStringLiteral("config"),
        QStringLiteral("Config file path"),
        QStringLiteral("path"));
    parser.addOption(configOption);
    parser.process(app);

    MainWindow window(parser.value(configOption));
    window.show();
    return app.exec();
}
