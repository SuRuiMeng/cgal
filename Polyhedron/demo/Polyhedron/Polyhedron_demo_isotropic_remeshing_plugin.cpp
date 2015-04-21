#include <QtCore/qglobal.h>

#include "Scene_polyhedron_item.h"
#include "Scene_polyhedron_selection_item.h"
#include "Polyhedron_demo_plugin_helper.h"
#include "Polyhedron_type.h"

#include <CGAL/iterator.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/boost/graph/graph_traits_Polyhedron_3.h>

#include <QTime>
#include <QAction>
#include <QMainWindow>
#include <QApplication>
#include <QInputDialog>
#include <QString>

#include <vector>
#include <algorithm>
#include <queue>
#include <sstream>

class Polyhedron_demo_isotropic_remeshing_plugin :
  public QObject,
  public Polyhedron_demo_plugin_helper
{
  Q_OBJECT
  Q_INTERFACES(Polyhedron_demo_plugin_interface)

public:
  void init(QMainWindow* mainWindow, Scene_interface* scene_interface)
  {
    this->scene = scene_interface;
    this->mw = mainWindow;
    actionIsotropicRemeshing_ = new QAction("Isotropic remeshing", mw);
    if (actionIsotropicRemeshing_) {
      connect(actionIsotropicRemeshing_, SIGNAL(triggered()),
        this, SLOT(isotropic_remeshing()));
    }
  }

  QList<QAction*> actions() const {
    return QList<QAction*>() << actionIsotropicRemeshing_;
  }

  bool applicable(QAction*) const
  {
    return qobject_cast<Scene_polyhedron_item*>(scene->item(scene->mainSelectionIndex()))
    || qobject_cast<Scene_polyhedron_selection_item*>(scene->item(scene->mainSelectionIndex()));
  }

public Q_SLOTS:
  void isotropic_remeshing()
  {
    const Scene_interface::Item_id index = scene->mainSelectionIndex();

    Scene_polyhedron_item* poly_item =
      qobject_cast<Scene_polyhedron_item*>(scene->item(index));

    Scene_polyhedron_selection_item* selection_item =
      qobject_cast<Scene_polyhedron_selection_item*>(scene->item(index));

    if (poly_item || selection_item)
    {
      // wait cursor
      QApplication::setOverrideCursor(Qt::WaitCursor);

      QTime time;
      time.start();

      double diago_length = (poly_item != NULL)
        ? poly_item->bbox().diagonal_length()
        : selection_item->bbox().diagonal_length();

      std::ostringstream oss;
      oss << "Target edge length?" << std::endl;
      oss << "  Diagonal length of the Bbox of the selection to remesh is ";
      oss << diago_length << std::endl;
      oss << "  (default is 5% of it)" << std::endl;

      bool ok;
      double target_length = QInputDialog::getDouble(this->mw,
        QString("Isotropic remeshing : Edge length"),
        QString::fromStdString(oss.str()),//question
        0.05 * diago_length,              //value
        1e-6 * diago_length,              //min
        2.   * diago_length,              //max
        3,                                //decimals
        &ok); //Qt::WindowFlags flags = 0);
      if (!ok)
        std::cout << "Remeshing aborted" << std::endl;

      Polyhedron *pRemeshed = new Polyhedron;
      if (selection_item) {
        Polyhedron* pMesh = selection_item->polyhedron();
        CGAL::Polygon_mesh_processing::incremental_triangle_based_remeshing(*pMesh
        , selection_item->selected_facets
        , target_length);
      }
      else if (poly_item){
        Polyhedron* pMesh = poly_item->polyhedron();
        CGAL::Polygon_mesh_processing::incremental_triangle_based_remeshing(*pMesh
        , faces(*pMesh)
        , target_length);
      }
      else{
        std::cout << "Can't remesh that type of thing" << std::endl;
      }
      std::cout << "ok (" << time.elapsed() << " ms)" << std::endl;

      Scene_polyhedron_item* new_item = new Scene_polyhedron_item(pRemeshed);
      new_item->setName(tr("%1 (iso remeshing)").arg(scene->item(index)->name()));
      new_item->setColor(Qt::magenta);
      new_item->setRenderingMode(FlatPlusEdges);
      scene->addItem(new_item);

      // default cursor
      QApplication::restoreOverrideCursor();
    }
  }

private:
  QAction* actionIsotropicRemeshing_;

}; // end Polyhedron_demo_isotropic_remeshing_plugin

Q_EXPORT_PLUGIN2(Polyhedron_demo_isotropic_remeshing_plugin,
                 Polyhedron_demo_isotropic_remeshing_plugin)

#include "Polyhedron_demo_isotropic_remeshing_plugin.moc"
