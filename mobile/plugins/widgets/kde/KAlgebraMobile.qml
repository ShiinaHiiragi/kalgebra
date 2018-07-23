import QtQuick 2.2
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.0 as Kirigami
import org.kde.analitza 1.0
import org.kde.kalgebra.mobile 1.0

Kirigami.ApplicationWindow
{
    id: rootItem
    height: 500
    width: 900
    visible: true

    contextDrawer: Kirigami.ContextDrawer {}

    globalDrawer: Kirigami.GlobalDrawer {
        id: drawer

        title: "KAlgebra"
        titleIcon: "qrc:/kalgebra.svgz"
        bannerImageSource: "qrc:/header.jpg"

        Instantiator {
            delegate: Kirigami.Action {
                text: title
                iconName: decoration
                onTriggered: {
                    var component = Qt.createComponent(model.path);
                    if (component.status == Component.Error) {
                        console.log("error", component.errorString());
                        return;
                    }

                    try {
                        rootItem.pageStack.replace(component, {title: title})
                    } catch(e) {
                        console.log("error", e)
                    }
                }
            }
            model: PluginsModel {}
            onObjectAdded: {
                var acts = [];
                for(var v in drawer.actions) {
                    acts.push(drawer.actions[v]);
                }
                acts.splice(index, 0, object)
                drawer.actions = acts;
            }
            onObjectRemoved: {
                var acts = [];
                for(var v in drawer.actions) {
                    acts.push(drawer.actions[v]);
                }
                drawer.actions.splice(drawer.actions.indexOf(object), 1)
                drawer.actions = acts;
            }
        }

        actions: []
    }

    Component.onCompleted: {
        drawer.actions[0].trigger()
    }
}
