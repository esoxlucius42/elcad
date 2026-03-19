#pragma once
#include <QString>
#include <QList>

#ifdef ELCAD_HAVE_OCCT
#include <TopoDS_Shape.hxx>
#endif

namespace elcad {

struct StepImportResult {
    bool                success{false};
    QString             errorMsg;
    QList<QString>      names;
#ifdef ELCAD_HAVE_OCCT
    QList<TopoDS_Shape> shapes;
#endif
};

struct StepExportResult {
    bool    success{false};
    QString errorMsg;
};

class StepIO {
public:
#ifdef ELCAD_HAVE_OCCT
    static StepImportResult importStep(const QString& filePath);
    static StepExportResult exportStep(const QString& filePath,
                                        const QList<TopoDS_Shape>& shapes,
                                        const QList<QString>& names);
#endif
};

} // namespace elcad
