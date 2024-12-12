/*************************************************************************************
 *  Copyright (C) 2007 by Aleix Pol <aleixpol@kde.org>                               *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include <sys/resource.h>
#include <sys/time.h>

#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include "http_message.h"
#include "http_server.h"
#include "kalgebra.h"
#include "kalgebra_version.h"

using simple_http_server::HttpMethod;
using simple_http_server::HttpRequest;
using simple_http_server::HttpResponse;
using simple_http_server::HttpServer;
using simple_http_server::HttpStatusCode;

KAlgebra *global_app = nullptr;

HttpResponse status_vars(const HttpRequest &_) {
    assert(global_app != nullptr);
    std::cout << "GET /vars"
              << "\n";

    HttpResponse response(HttpStatusCode::Ok);
    response.SetHeader("Content-Type", "application/json");
    response.SetContent(global_app->status_vars());
    return response;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("kalgebra");
    KAboutData about(QStringLiteral("kalgebra"),
        QStringLiteral("KAlgebra"),
        QStringLiteral(KALGEBRA_VERSION_STRING),
        i18n("A portable calculator"),
        KAboutLicense::GPL,
        i18n("(C) 2006-2016 Aleix Pol i Gonzalez"));
    about.addAuthor(i18n("Aleix Pol i Gonzalez"), QString(), QStringLiteral("aleixpol@kde.org"));
    about.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    KAboutData::setApplicationData(about);

    {
        QCommandLineParser parser;
        about.setupCommandLine(&parser);
        parser.process(app);
        about.processCommandLine(&parser);
    }
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("kalgebra")));

    KAlgebra widget;
    widget.show();
    global_app = &widget;

    std::string host = "0.0.0.0";
    int port = 8080;
    HttpServer server(host, port);

    server.RegisterHttpRequestHandler("/vars", HttpMethod::HEAD, status_vars);
    server.RegisterHttpRequestHandler("/vars", HttpMethod::GET, status_vars);

    server.Start();
    std::cout << "Server listening on " << host << ":" << port << std::endl;
    return app.exec();
}
