#ifdef ELCAD_HAVE_OCCT
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <TopLoc_Location.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax2.hxx>
#endif

#include <iostream>

int sphere_tess_count_main(int argc, char** argv)
{
    (void)argc; (void)argv;
    std::cout << "sphere_tess_count: testing tessellation of a sphere (r=30)" << std::endl;
#ifdef ELCAD_HAVE_OCCT
    gp_Ax2 ax(gp_Pnt(0,0,0), gp_Dir(0,0,1));
    TopoDS_Shape sphere = BRepPrimAPI_MakeSphere(ax, 30.0).Shape();
    double defs[] = {0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0};
    for (double d : defs) {
        // Tessellate and count triangles
        BRepMesh_IncrementalMesh mesh(sphere, static_cast<float>(d), Standard_False, 0.5, Standard_True);
        int total = 0;
        for (TopExp_Explorer exp(sphere, TopAbs_FACE); exp.More(); exp.Next()) {
            const TopoDS_Face& face = TopoDS::Face(exp.Current());
            TopLoc_Location loc;
            Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
            if (tri.IsNull()) continue;
            total += tri->NbTriangles();
        }
        std::cout << "deflection=" << d << " -> triangles=" << total << std::endl;
    }
#else
    std::cout << "OpenCASCADE not available in this build." << std::endl;
#endif
    return 0;
}
