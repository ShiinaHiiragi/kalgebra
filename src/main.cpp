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

#include <qobjectdefs.h>
#define SERVER_VERSION "0.3"

#include <sys/resource.h>
#include <sys/time.h>

#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>
#include <QColor>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include "json.h"
#include "http_message.h"
#include "http_server.h"
#include "kalgebra.h"
#include "kalgebra_version.h"

using simple_http_server::HttpMethod;
using simple_http_server::HttpRequest;
using simple_http_server::HttpResponse;
using simple_http_server::HttpServer;
using simple_http_server::HttpStatusCode;
using json = nlohmann::json;

KAlgebra *global_app = nullptr;

HttpResponse status_version(const HttpRequest &_) {
    assert(global_app != nullptr);
    std::cout << "GET /version" << "\n";

    HttpResponse response(HttpStatusCode::Ok);
    response.SetHeader("Content-Type", "application/json");
    response.SetContent(SERVER_VERSION);
    return response;
}

HttpResponse status_vars(const HttpRequest &_) {
    assert(global_app != nullptr);
    std::cout << "GET /vars" << "\n";

    HttpResponse response(HttpStatusCode::Ok);
    response.SetHeader("Content-Type", "application/json");
    response.SetContent(global_app->status_vars());
    return response;
}

HttpResponse status_func2d(const HttpRequest &request) {
    assert(global_app != nullptr);
    std::string query = request.content();
    std::cout << "POST /func/2d with " << query << "\n";

    try {
        HttpResponse response(HttpStatusCode::Ok);
        response.SetHeader("Content-Type", "application/json");
        response.SetContent(global_app->status_func2d(json::parse(query)));
        return response;
    } catch (...) {
        HttpResponse response(HttpStatusCode::BadRequest);
        return response;
    }
}

HttpResponse status_func3d(const HttpRequest &request) {
    assert(global_app != nullptr);
    std::string query = request.content();
    std::cout << "POST /func/3d with " << query << "\n";

    try {
        HttpResponse response(HttpStatusCode::Ok);
        response.SetHeader("Content-Type", "application/json");
        response.SetContent(global_app->status_func3d(json::parse(query)));
        return response;
    } catch (...) {
        HttpResponse response(HttpStatusCode::BadRequest);
        return response;
    }
}

HttpResponse operate_tab(const HttpRequest &request) {
    assert(global_app != nullptr);
    std::string query = request.content();
    std::cout << "POST /tab with " << query << "\n";

    try {
        HttpResponse response(HttpStatusCode::Ok);
        response.SetHeader("Content-Type", "application/json");
        global_app->operate_tab(std::stoi(query));
        response.SetContent("OK");
        return response;
    } catch (...) {
        HttpResponse response(HttpStatusCode::BadRequest);
        return response;
    }
}

HttpResponse operate_add2d(const HttpRequest &request) {
    assert(global_app != nullptr);
    std::string query = request.content();
    std::cout << "POST /add/2d" << "\n";

    QMetaObject::invokeMethod(
        global_app,
        "operate_add2d",
        Qt::QueuedConnection,
        Q_ARG(std::string, query)
    );

    HttpResponse response(HttpStatusCode::Ok);
    response.SetHeader("Content-Type", "application/json");
    response.SetContent("OK");
    return response;
}

HttpResponse operate_add3d(const HttpRequest &request) {
    assert(global_app != nullptr);
    std::string query = request.content();
    std::cout << "POST /add/2d" << "\n";

    QMetaObject::invokeMethod(
        global_app,
        "operate_add3d",
        Qt::QueuedConnection,
        Q_ARG(std::string, query)
    );

    HttpResponse response(HttpStatusCode::Ok);
    response.SetHeader("Content-Type", "application/json");
    response.SetContent("OK");
    return response;
}

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

    // remove strange shadow from menubar
    QPalette palette = app.palette();
    palette.setColor(QPalette::Disabled, QPalette::Window, QColor("#FAFAFA"));
    app.setPalette(palette);

    int port = 8000;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    std::string host = "0.0.0.0";
    HttpServer server(host, port);

    server.RegisterHttpRequestHandler("/version", HttpMethod::HEAD, status_version);
    server.RegisterHttpRequestHandler("/version", HttpMethod::GET, status_version);

    server.RegisterHttpRequestHandler("/vars", HttpMethod::HEAD, status_vars);
    server.RegisterHttpRequestHandler("/vars", HttpMethod::GET, status_vars);

    server.RegisterHttpRequestHandler("/func/2d", HttpMethod::HEAD, status_func2d);
    server.RegisterHttpRequestHandler("/func/2d", HttpMethod::POST, status_func2d);

    server.RegisterHttpRequestHandler("/func/3d", HttpMethod::HEAD, status_func3d);
    server.RegisterHttpRequestHandler("/func/3d", HttpMethod::POST, status_func3d);

    server.RegisterHttpRequestHandler("/tab", HttpMethod::HEAD, operate_tab);
    server.RegisterHttpRequestHandler("/tab", HttpMethod::POST, operate_tab);

    server.RegisterHttpRequestHandler("/add/2d", HttpMethod::HEAD, operate_add2d);
    server.RegisterHttpRequestHandler("/add/2d", HttpMethod::POST, operate_add2d);

    server.RegisterHttpRequestHandler("/add/3d", HttpMethod::HEAD, operate_add3d);
    server.RegisterHttpRequestHandler("/add/3d", HttpMethod::POST, operate_add3d);

    server.Start();
    std::cout << "Server listening on " << host << ":" << port << std::endl;
    return app.exec();
}
