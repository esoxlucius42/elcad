#include "document/StepIO.h"
#include "core/Logger.h"

#ifdef ELCAD_HAVE_OCCT

#include <STEPCAFControl_Writer.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <STEPControl_Reader.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Interface_Static.hxx>

namespace elcad {

StepImportResult StepIO::importStep(const QString& filePath)
{
    StepImportResult result;
    LOG_INFO("STEP import: reading '{}'", filePath.toStdString());

    STEPControl_Reader reader;
    IFSelect_ReturnStatus status = reader.ReadFile(filePath.toUtf8().constData());
    if (status != IFSelect_RetDone) {
        result.errorMsg = "STEPControl_Reader::ReadFile failed";
        LOG_ERROR("STEP import failed — file='{}' — ReadFile returned status {}; "
                  "possible causes: file not found, permission denied, or corrupt/invalid STEP data",
                  filePath.toStdString(), static_cast<int>(status));
        return result;
    }

    reader.TransferRoots();
    int nShapes = reader.NbShapes();
    if (nShapes == 0) {
        result.errorMsg = "No shapes found in STEP file";
        LOG_ERROR("STEP import failed — file='{}' — file was read successfully but "
                  "TransferRoots() produced 0 shapes; the file may contain only "
                  "metadata, annotations, or unsupported entity types",
                  filePath.toStdString());
        return result;
    }

    LOG_INFO("STEP import: transferring {} shape(s) from '{}'", nShapes, filePath.toStdString());
    for (int i = 1; i <= nShapes; ++i) {
        TopoDS_Shape shape = reader.Shape(i);
        if (shape.IsNull()) {
            LOG_WARN("STEP import: shape {} of {} is null — skipping "
                     "(may be an empty compound or unsupported solid type)",
                     i, nShapes);
            continue;
        }
        result.shapes.append(shape);
        result.names.append(QString("Import_%1").arg(i));
        LOG_DEBUG("STEP import: shape {} accepted — type={}",
                  i, static_cast<int>(shape.ShapeType()));
    }

    result.success = !result.shapes.isEmpty();
    if (!result.success) {
        result.errorMsg = "All shapes were null";
        LOG_ERROR("STEP import failed — file='{}' — all {} shape(s) returned by "
                  "the reader were null after transfer",
                  filePath.toStdString(), nShapes);
    } else {
        LOG_INFO("STEP import succeeded — file='{}' — {} of {} shape(s) imported",
                 filePath.toStdString(), result.shapes.size(), nShapes);
    }

    return result;
}

StepExportResult StepIO::exportStep(const QString& filePath,
                                     const QList<TopoDS_Shape>& shapes,
                                     const QList<QString>& names)
{
    StepExportResult result;

    if (shapes.isEmpty()) {
        result.errorMsg = "No shapes to export";
        LOG_WARN("STEP export aborted — shapes list is empty, nothing to write to '{}'",
                 filePath.toStdString());
        return result;
    }

    LOG_INFO("STEP export: writing {} body/bodies to '{}'",
             shapes.size(), filePath.toStdString());

    STEPControl_Writer writer;

    // Set mm units
    Interface_Static::SetCVal("write.step.unit", "MM");
    Interface_Static::SetCVal("write.step.schema", "AP203");

    for (int i = 0; i < shapes.size(); ++i) {
        if (shapes[i].IsNull()) {
            LOG_WARN("STEP export: body '{}' (index {}) has a null shape — skipping",
                     names.value(i).toStdString(), i);
            continue;
        }
        IFSelect_ReturnStatus s = writer.Transfer(shapes[i], STEPControl_AsIs);
        if (s != IFSelect_RetDone && s != IFSelect_RetVoid) {
            result.errorMsg = QString("Transfer failed for body '%1'").arg(names.value(i));
            LOG_ERROR("STEP export failed — body='{}' (index {}) transfer returned "
                      "status {}; the shape may have invalid topology or unsupported geometry types",
                      names.value(i).toStdString(), i, static_cast<int>(s));
            return result;
        }
        LOG_DEBUG("STEP export: transferred body '{}' (index {})",
                  names.value(i).toStdString(), i);
    }

    IFSelect_ReturnStatus writeStatus = writer.Write(filePath.toUtf8().constData());
    if (writeStatus != IFSelect_RetDone) {
        result.errorMsg = "STEPControl_Writer::Write failed";
        LOG_ERROR("STEP export failed — file='{}' — Write returned status {}; "
                  "possible causes: path not writable, disk full, or invalid file name",
                  filePath.toStdString(), static_cast<int>(writeStatus));
        return result;
    }

    result.success = true;
    LOG_INFO("STEP export succeeded — file='{}'", filePath.toStdString());
    return result;
}

} // namespace elcad

#endif // ELCAD_HAVE_OCCT
