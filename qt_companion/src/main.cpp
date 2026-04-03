#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTimer>
#include <iostream>

#include "companionbackend.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("EmoBot 梦幻陪伴系统");
    parser.addHelpOption();
    parser.addOption(QCommandLineOption("selftest", "Run an end-to-end backend self test (no UI)."));
    parser.process(app);

    CompanionBackend backend;

    if (parser.isSet("selftest")) {
        // Quick sanity check: verify embedded avatar resources load.
        {
            QPixmap userPx(QStringLiteral(":/resources/avatars/user.png"));
            QPixmap xiaPx(QStringLiteral(":/resources/avatars/xia.png"));
            std::cout << "[SelfTest][Avatar] userNull=" << (userPx.isNull() ? "true" : "false")
                      << " xiaNull=" << (xiaPx.isNull() ? "true" : "false") << std::endl;
        }

        QObject::connect(&backend, &CompanionBackend::assistantReplyReady,
                         &app, [&](const QString &reply, const QString &emotion, const QString &gesture) {
                             std::cout << "[SelfTest] reply=" << reply.toStdString()
                                       << " emotion=" << emotion.toStdString()
                                       << " gesture=" << gesture.toStdString() << std::endl;
                             app.quit();
                         });

        QObject::connect(&backend, &CompanionBackend::errorOccurred,
                         &app, [&](const QString &message) {
                             std::cout << "[SelfTest][Error] " << message.toStdString() << std::endl;
                             app.quit();
                         });

        // Give the event loop one tick.
        QTimer::singleShot(0, [&]() {
            backend.sendMessage(QStringLiteral("你好，哥！"), backend.defaultPersonaText());
        });

        return app.exec();
    }

    MainWindow w(&backend);
    w.show();
    return app.exec();
}

