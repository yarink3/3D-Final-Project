// This file is part of libigl, a simple c++ geometry processing library.
//
// Copyright (C) 2014 Daniele Panozzo <daniele.panozzo@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.

#include "Viewer.h"
//#include "/home/omer/CLionProjects/Final Project Snake/tutorial/sandBox/AudioPlayer.h"
#include <igl/collapse_edge.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <limits>
#include <cassert>
#include "SDL2/SDL.h"

#include <igl/project.h>
//#include <igl/get_seconds.h>
#include <igl/readOBJ.h>
#include <igl/readOFF.h>
#include <igl/adjacency_list.h>
#include <igl/writeOBJ.h>
#include <igl/writeOFF.h>
#include <igl/massmatrix.h>
#include <igl/file_dialog_open.h>
#include <igl/file_dialog_save.h>
#include <igl/quat_mult.h>
#include <igl/axis_angle_to_quat.h>
#include <igl/trackball.h>
#include <igl/two_axis_valuator_fixed_up.h>
#include <igl/snap_to_canonical_view_quat.h>
#include <igl/unproject.h>
#include <igl/serialize.h>
#include <iostream>
#include <fstream>
#include <string>
#include <igl/AABB.h>
#include <X11/Xlib.h>
#include <igl/png/readPNG.h>



using namespace std;


static double highdpi = 1;
static double scroll_x = 0;
static double scroll_y = 0;


namespace igl {
    namespace opengl {
        namespace glfw {

                IGL_INLINE void Viewer::init(int newLevel) {

                levelNumber=newLevel;
                userPoints=(levelNumber-1)*25;
                std::cout <<"PLAYER POINTS SO FAR: "<< userPoints << std::endl;


                start = time(0);
                    string line;
                ifstream myfile("configuration4.txt");
                    if (myfile.is_open()) {
                        while (myfile.good()) {
                            getline(myfile, line);
                            if(line!="") {
                                line = line.substr(0, line.length() - 1);
                                int i ;
                                if ( line.substr(line.length() - 13, line.length() - 1) == "ycylinder.obj") {

                                    i = 14;
                                }
                                else {
                                    i = 8;

                                }



                                for (int k = 0; k < i; k++) {

                                    if(firstTime==false) {
                                        if(i!=14) {

                                            load_mesh_from_file(line);
                                        }
                                    }
                                    else {
                                        load_mesh_from_file(line);
                                    }

                                    if(k==i-1 && i!=14) {
                                        meshes[meshes.size() - 1].pointsValue = meshes[meshes.size() - 1].pointsValue * (-1);
                                        drawInGreen(meshes.size() - 1);

                                    }
                                }
                            }

                        }
                        if(levelNumber==1 ||finishedGameByCheating) {
                            clearDataAndMeshes();
                            finishedGameByCheating=false;
                        }
                        if(firstTime)
                       {
                           std::string p = "/home/omer/CLionProjects/Final-Project-Snake/tutorial/MusicWav/";
                           player = AudioPlayer(p);
                           player.playBackground();

                            for(int j=0;j<numOfCylinder;j++) {

                                firstTime=false;

                                data(j).show_lines = false;
                                Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> R, G, B, A;
                                igl::png::readPNG("snaketexture.png", R, G, B, A);
                                data(j).show_texture = true;
                                data(j).set_texture(R, G, B);
                            }
                       }

                        for (int j = numOfCylinder; j < meshes.size(); j++)
                            meshes[j].vectorToMoveBy = meshes[j].FirstvectorToMoveBy * levelNumber;

                        for(int j=numOfCylinder;j<meshes.size();j++)
                            if(meshes[j].pointsValue<0)
                                drawInGreen(j);
                            else
                                drawInYellow(j);
                        myfile.close();
                    }
                }


            //IGL_INLINE void Viewer::init_plugins()
            //{
            //  // Init all plugins
            //  for (unsigned int i = 0; i<plugins.size(); ++i)
            //  {
            //    plugins[i]->init(this);
            //  }
            //}

            //IGL_INLINE void Viewer::shutdown_plugins()
            //{
            //  for (unsigned int i = 0; i<plugins.size(); ++i)
            //  {
            //    plugins[i]->shutdown();
            //  }
            //}

            IGL_INLINE Viewer::Viewer():
                    data_list(1),
                    selected_data_index(0),
                    next_data_id(1)
            {
                data_list.front().id = 0;



                // Temporary variables initialization
                // down = false;
                //  hack_never_moved = true;
                scroll_position = 0.0f;

                // Per face
                data().set_face_based(false);


#ifndef IGL_VIEWER_VIEWER_QUIET
                const std::string usage(R"(igl::opengl::glfw::Viewer usage:
  [drag]  Rotate scene
  A,a     Toggle animation (tight draw loop)
  F,f     Toggle face based
  I,i     Toggle invert normals
  L,l     Toggle wireframe
  O,o     Toggle orthographic/perspective projection
  T,t     Toggle filled faces
  [,]     Toggle between cameras
  1,2     Toggle between models
  ;       Toggle vertex labels
  :       Toggle face labels)"
                );
#endif
            }

            IGL_INLINE Viewer::~Viewer() {
                int size=meshes.size();
                for(int i=0;i<meshes.size();i++) {
                    delete &meshes[i];
                }

                delete &meshes;

            }

            IGL_INLINE bool Viewer::load_mesh_from_file(
                    const std::string &mesh_file_name_string) {

                // Create new data slot and set to selected
                if (!(data().F.rows() == 0 && data().V.rows() == 0)) {
                    append_mesh();
                }
                data().clear();

                size_t last_dot = mesh_file_name_string.rfind('.');
                if (last_dot == std::string::npos) {
                    std::cerr << "Error: No file extension found in " <<
                              mesh_file_name_string << std::endl;
                    return false;
                }

                std::string extension = mesh_file_name_string.substr(last_dot + 1);
                if (extension == "off" || extension == "OFF") {
                    OurMesh *mesh = new OurMesh();
                    if(meshes.size()%5==0) {
                        mesh->vectorToMoveBy << 0.11, 0, 0;
                        mesh->vectorToMoveBy = mesh->vectorToMoveBy * levelNumber;
                    }
                    else {
                        if (meshes.size() % 5 == 1) {
                            mesh->vectorToMoveBy << 0, 0.11, 0;
//                            mesh->vectorToMoveBy = mesh->vectorToMoveBy * levelNumber;
                        }

                        else if(meshes.size() % 5 == 2) {
                            mesh->vectorToMoveBy << 0.11, 0.11, 0.11;
//                            mesh->vectorToMoveBy = mesh->vectorToMoveBy * levelNumber;
                        }

                        else if(meshes.size() % 5 == 3) {
                            mesh->vectorToMoveBy << 0,-0.11,-0.11;
//                            mesh->vectorToMoveBy = mesh->vectorToMoveBy * levelNumber;
                        }
                        else if(meshes.size() % 5 == 4) {
                            mesh->vectorToMoveBy << 0.11, 0, 0.11;
//                            mesh->vectorToMoveBy = mesh->vectorToMoveBy * levelNumber;
                        }
                    }
//                    std::cout<<mesh->vectorToMoveBy<<std::endl;

                    if (!igl::readOFF(mesh_file_name_string, mesh->V, mesh->F))
                        return false;

                    igl::AABB<Eigen::MatrixXd, 3> tree;
                    tree.init(mesh->V,mesh->F);

                    mesh->KDtree=tree;

                    data().set_mesh(mesh->V, mesh->F);
                    data().point_size=10;

                    mesh->facesToNormals = &data().F_normals;
                    mesh->init();

//                    drawBox(&tree.m_box,meshes.size()-1,Eigen::RowVector3d(1,0,0));
                    if(mesh_file_name_string.substr(mesh_file_name_string.length()-9,mesh_file_name_string.length()-1)=="bunny.off") {
                        std::string check=mesh_file_name_string.substr(mesh_file_name_string.length()-9,mesh_file_name_string.length()-1);

                        mesh->type = "bunny";
                        mesh->pointsValue=3;

                    }

                    else {
                        if (mesh_file_name_string.substr(mesh_file_name_string.length() - 7,
                                                         mesh_file_name_string.length() - 1) == "cow.off") {
                            std::string check = mesh_file_name_string.substr(mesh_file_name_string.length() - 7,
                                                                             mesh_file_name_string.length() - 1);

                            mesh->type = "cow";
                            mesh->pointsValue = 7;

                        } else if (mesh_file_name_string.substr(mesh_file_name_string.length() - 15,mesh_file_name_string.length() - 1) ==
                                   "cheburashka.off") {
                            mesh->type = "cheburashka";
                            mesh->pointsValue = 10;

                        }
                    }
                    mesh->shouldMove=true;
                    data().MyTranslate(getRandomLocation(),true);

                    meshes.push_back(*mesh);


                } else if (extension == "obj" || extension == "OBJ") {
                    OurMesh *mesh = new OurMesh();

                    if(meshes.size()%5==0) {
                        mesh->vectorToMoveBy << 0.11, 0, 0;
//                        mesh->vectorToMoveBy = mesh->vectorToMoveBy * levelNumber;
                    }
                    else {
                        if (meshes.size() % 5 == 1) {
                            mesh->vectorToMoveBy << 0, 0.11, 0;
//                            mesh->vectorToMoveBy = mesh->vectorToMoveBy * levelNumber;
                        }

                        else if(meshes.size() % 5 == 2) {
                            mesh->vectorToMoveBy << 0.11, 0.11, 0.11;
//                            mesh->vectorToMoveBy = mesh->vectorToMoveBy * levelNumber;
                        }

                        else if(meshes.size() % 5 == 3) {
                            mesh->vectorToMoveBy << 0,-0.11,-0.11;
//                            mesh->vectorToMoveBy = mesh->vectorToMoveBy * levelNumber;
                        }
                        else if(meshes.size() % 5 == 4) {
                            mesh->vectorToMoveBy << 0.11, 0, 0.11;
//                            mesh->vectorToMoveBy = mesh->vectorToMoveBy * levelNumber;
                        }
                    }
//                    std::cout<<mesh->vectorToMoveBy.transpose()<<std::endl;

                    Eigen::MatrixXd corner_normals;
                    Eigen::MatrixXi fNormIndices;

                    Eigen::MatrixXd UV_V;
                    Eigen::MatrixXi UV_F;


                    if (!(
                            igl::readOBJ(
                                    mesh_file_name_string,
                                    mesh->V, UV_V, corner_normals, mesh->F, UV_F, fNormIndices))) {
                        return false;
                    }



                    if(mesh_file_name_string.substr(mesh_file_name_string.length()-13,mesh_file_name_string.length()-1)=="ycylinder.obj")
                    { // isYcylinder
                        mesh->index=numOfCylinder;

                        Eigen::Vector3d m1 = mesh->V.colwise().minCoeff(); // draw the axises
                        Eigen::Vector3d M1 = mesh->V.colwise().maxCoeff();

                        data().Tout.translate(Eigen::Vector3f (0,1.6,0));

                        Eigen::Vector3d m = mesh->V.colwise().minCoeff();
                        Eigen::Vector3d M = mesh->V.colwise().maxCoeff();

                        if(numOfCylinder==0) {  // we are in the first link
                            mesh->parent = 0;
                            data().Tout.translate(Eigen::Vector3f (0,-1.6,0));
                        }

                        else {
                            mesh->parent = numOfCylinder - 1;
                        }
                        mesh->centerOfRotation << Eigen::Vector3d((M(0)+m(0))/2,m(1),(M(2)+m(2))/2);
                        data().SetCenterOfRotation(Eigen::Vector3f((M(0)+m(0))/2,m(1),(M(2)+m(2))/2));

                        // draw thw axis
                        if(numOfCylinder!=13){

                            Eigen::MatrixXd V_box(6,3);
                            V_box <<
                                  (m(0)+M(0))/2, M(1),m(2)-1.6,// 1
                                    M(0)-1.6,M(1),(m(2)+M(2))/2, //2
                                    m(0)+1.6,M(1),(m(2)+M(2))/2, //3
                                    (m(0)+M(0))/2, M(1),M(2)+1.6,// 4
                                    (M(0)+m(0))/2,m(0),(M(2)+m(2))/2,
                                    0,2.5,0;


                            Eigen::MatrixXi E_box(3,2);
                            E_box <<
                                  0,3,
                                    1,2,
                                    4,5;

                                data().add_edges(V_box.row(E_box(0, 0)),V_box.row(E_box(0, 1)),Eigen::RowVector3d(1, 0, 0));
                                data().add_edges(V_box.row(E_box(1, 0)),V_box.row(E_box(1, 1)),Eigen::RowVector3d(0, 1, 0));
                                data().add_edges(V_box.row(E_box(2, 0)),V_box.row(E_box(2, 1)),Eigen::RowVector3d(0, 0, 1));

                        }
                        numOfCylinder++;

                        mesh->type="Ycylinder";
                        mesh->pointsValue=0;


                    }
                    else {// not Ycylinder
                        std::string check=mesh_file_name_string.substr(mesh_file_name_string.length()-10,mesh_file_name_string.length()-1);
                        if(mesh_file_name_string.substr(mesh_file_name_string.length()-10,mesh_file_name_string.length()-1)=="sphere.obj") {
                            mesh->type = "sphere";
                            mesh->pointsValue=5;

                        }



                        mesh->shouldMove=true;
                        data().MyTranslate(getRandomLocation(),true);
                    }




                    igl::AABB<Eigen::MatrixXd, 3> tree;
                    tree.init(mesh->V, mesh->F);

                    mesh->KDtree=tree;

                    data().set_mesh(mesh->V, mesh->F);
                    data().set_uv(UV_V, UV_F);
                    data().show_overlay_depth=false;
                    data().line_width=3;
                    data().point_size=10;
                    mesh->facesToNormals = &data().F_normals;
                    mesh->init();
                    meshes.push_back(*mesh);


                } else {
                    // unrecognized file type
                    printf("Error: %s is not a recognized file type.\n", extension.c_str());
                    return false;
                }

                data().compute_normals();
                data().uniform_colors(Eigen::Vector3d(51.0 / 255.0, 43.0 / 255.0, 33.3 / 255.0),
                                      Eigen::Vector3d(255.0 / 255.0, 228.0 / 255.0, 58.0 / 255.0),
                                      Eigen::Vector3d(255.0 / 255.0, 235.0 / 255.0, 80.0 / 255.0));

                // Alec: why?
                if (data().V_uv.rows() == 0) {
                    data().grid_texture();
                }


                //for (unsigned int i = 0; i<plugins.size(); ++i)
                //  if (plugins[i]->post_load())
                //    return true;

                return true;
            }

            IGL_INLINE bool Viewer::save_mesh_to_file(
                    const std::string &mesh_file_name_string) {
                // first try to load it with a plugin
                //for (unsigned int i = 0; i<plugins.size(); ++i)
                //  if (plugins[i]->save(mesh_file_name_string))
                //    return true;

                size_t last_dot = mesh_file_name_string.rfind('.');
                if (last_dot == std::string::npos) {
                    // No file type determined
                    std::cerr << "Error: No file extension found in " <<
                              mesh_file_name_string << std::endl;
                    return false;
                }
                std::string extension = mesh_file_name_string.substr(last_dot + 1);
                if (extension == "off" || extension == "OFF") {
                    return igl::writeOFF(
                            mesh_file_name_string, data().V, data().F);
                } else if (extension == "obj" || extension == "OBJ") {
                    Eigen::MatrixXd corner_normals;
                    Eigen::MatrixXi fNormIndices;

                    Eigen::MatrixXd UV_V;
                    Eigen::MatrixXi UV_F;

                    return igl::writeOBJ(mesh_file_name_string,
                                         data().V,
                                         data().F,
                                         corner_normals, fNormIndices, UV_V, UV_F);
                } else {
                    // unrecognized file type
                    printf("Error: %s is not a recognized file type.\n", extension.c_str());
                    return false;
                }
                return true;
            }

            IGL_INLINE bool Viewer::load_scene() {
                std::string fname = igl::file_dialog_open();
                if (fname.length() == 0)
                    return false;
                return load_scene(fname);
            }

            IGL_INLINE bool Viewer::load_scene(std::string fname) {
                // igl::deserialize(core(),"Core",fname.c_str());
                igl::deserialize(data(), "Data", fname.c_str());
                return true;
            }

            IGL_INLINE bool Viewer::save_scene() {
                std::string fname = igl::file_dialog_save();
                if (fname.length() == 0)
                    return false;
                return save_scene(fname);
            }

            IGL_INLINE bool Viewer::save_scene(std::string fname) {
                //igl::serialize(core(),"Core",fname.c_str(),true);
                igl::serialize(data(), "Data", fname.c_str());

                return true;
            }

            IGL_INLINE void Viewer::open_dialog_load_mesh() {
                std::string fname = igl::file_dialog_open();

                if (fname.length() == 0)
                    return;

                this->load_mesh_from_file(fname.c_str());
            }

            IGL_INLINE void Viewer::open_dialog_save_mesh() {
                std::string fname = igl::file_dialog_save();

                if (fname.length() == 0)
                    return;

                this->save_mesh_to_file(fname.c_str());
            }

            IGL_INLINE ViewerData &Viewer::data(int mesh_id /*= -1*/) {
                assert(!data_list.empty() && "data_list should never be empty");
                int index;
                if (mesh_id == -1)
                    index = selected_data_index;
                else
                    index = mesh_index(mesh_id);

                assert((index >= 0 && index < data_list.size()) &&
                       "selected_data_index or mesh_id should be in bounds");
                return data_list[index];
            }

            IGL_INLINE const ViewerData &Viewer::data(int mesh_id /*= -1*/) const {
                assert(!data_list.empty() && "data_list should never be empty");
                int index;
                if (mesh_id == -1)
                    index = selected_data_index;
                else
                    index = mesh_index(mesh_id);

                assert((index >= 0 && index < data_list.size()) &&
                       "selected_data_index or mesh_id should be in bounds");
                return data_list[index];
            }

            IGL_INLINE int Viewer::append_mesh(bool visible /*= true*/) {
                assert(data_list.size() >= 1);

                data_list.emplace_back();
                selected_data_index = data_list.size() - 1;
                data_list.back().id = next_data_id++;
                //if (visible)
                //    for (int i = 0; i < core_list.size(); i++)
                //        data_list.back().set_visible(true, core_list[i].id);
                //else
                //    data_list.back().is_visible = 0;
                return data_list.back().id;
            }

            IGL_INLINE bool Viewer::erase_mesh(const size_t index) {
                assert((index >= 0 && index < data_list.size()) && "index should be in bounds");
                assert(data_list.size() >= 1);
                if (data_list.size() == 1) {
                    // Cannot remove last mesh
                    return false;
                }
                data_list[index].meshgl.free();
                data_list.erase(data_list.begin() + index);
                if (selected_data_index >= index && selected_data_index > 0) {
                    selected_data_index--;
                }

                return true;
            }

            IGL_INLINE size_t Viewer::mesh_index(const int id) const {
                for (size_t i = 0; i < data_list.size(); ++i) {
                    if (data_list[i].id == id)
                        return i;
                }
                return 0;
            }

            IGL_INLINE void Viewer::collapse() {
                auto PCOST = [=](
                        const int e,
                        const Eigen::MatrixXd &V,
                        const Eigen::MatrixXi &F/*F*/,
                        const Eigen::MatrixXi &E,
                        const Eigen::VectorXi &EMAP/*EMAP*/,
                        const Eigen::MatrixXi &EF/*EF*/,
                        const Eigen::MatrixXi &EI/*EI*/,
                        double &cost,
                        Eigen::RowVectorXd &p) {
                    std::vector<int> p1  =
                            igl::circulation(e,0 ,EMAP ,EF,EI);
                    Eigen::Matrix4d newQ1 = Eigen::Matrix4d::Zero() ;

                    for(int i=0; i<p1.size();i++){
                        Eigen::Vector3d n=data().F_normals.row(p1[i]).normalized();
                        Eigen::Vector3d B=V.row(E(e,0));
                        double a=n(0);
                        double b=n(1);
                        double c=n(2);
                        double d =(-1)*(B(0)*a +B(1)*b +B(2)*c);
                        Eigen:: Vector4d p1=Eigen::Vector4d (a,b,c,d);
                        Eigen:: Matrix4d p2=p1*p1.transpose();
                        newQ1+=p2;

                    }
                    std::vector<int> p2  =
                            igl::circulation(e,1 ,EMAP ,EF,EI);

                    for(int i=0; i<p2.size();i++) {
                        Eigen::Vector3d n = data().F_normals.row(p2[i]).normalized();
                        Eigen::Vector3d B = V.row(E(e, 1));
                        double a = n(0);
                        double b = n(1);
                        double c = n(2);
                        double d = (-1) * (B(0) * a + B(1) * b + B(2) * c);

                        Eigen::Vector4d p1 = Eigen::Vector4d(a, b, c, d);
                        Eigen::Matrix4d p2 = p1 * p1.transpose();
                        newQ1 += p2;
                    }

                    Eigen::Vector4d toRet;
                    Eigen::Matrix4d Qtag= Eigen::Matrix4d::Zero(),newQ3= Eigen::Matrix4d::Zero() ;

                    Qtag << newQ1(0,0), newQ1(0,1), newQ1(0,2),newQ1(0,3),
                            newQ1(0,1), newQ1(1,1), newQ1(1,2),newQ1(1,3),
                            newQ1(0,2), newQ1(1,2), newQ1(2,2),newQ1(2,3),
                            0,                0,               0,                     1;
                    newQ3=Qtag.inverse();
                    toRet = newQ3*Eigen::Vector4d(0 ,0 ,0, 1); // this is v tag.

                    cost= toRet.transpose()*newQ1*toRet;
                    p = Eigen::Vector3d(toRet[0], toRet[1], toRet[2]);
                    std::cout<<"edge < " << e <<" > , cost = <"<<cost << "> new v position (<" <<p[0]<< ">,<" <<p[1] <<">,<"<<p[2]<<">)"<<std::endl;

                };
                using namespace std;
                using namespace Eigen;
                using namespace igl;
                // If animating then collapse 10% of edges

                int i = selected_data_index;
                if (!meshes[i].Q->empty()) {
                    bool something_collapsed = false;
                    // collapse edge
                    const int max_iter = std::ceil(0.05 * meshes[i].Q->size());
                    for (int j = 0; j < max_iter; j++) {
                        if (!collapse_edge(
                                PCOST, meshes[i].V,
                                meshes[i].F,
                                meshes[i].E,
                                meshes[i].EMAP,
                                meshes[i].EF,
                                meshes[i].EI,
                                *meshes[i].Q,
                                *meshes[i].Qit,
                                meshes[i].C))
                        {
                            break;
                        }

                        something_collapsed = true;
                        meshes[i].numcollapsed++;

                    }

                    if (something_collapsed) {

                        std::cout << "collapsed:    " << meshes[i].numcollapsed << std::endl;
                        data().clear();
                        data().set_mesh(meshes[i].V, meshes[i].F);
                        data().set_face_based(true);
                    }
                }
            }

            Eigen:: Vector3f Viewer:: getNewRotationVector(int indexOfLink){
                Eigen::Vector3f vecY=Eigen::Vector3f (0,1,0);
                Eigen:: Matrix3f mat=Eigen::Matrix3f::Identity();
                for(int i=0; i<indexOfLink+1;i++){
                    mat=mat*data(i).Tout.rotation().matrix();

                }
                return mat*vecY;

            }

            Eigen:: Matrix4f Viewer:: getParentsTrans(int indexOfLink){
                Eigen:: Matrix4f mat=Eigen:: Matrix4f::Identity();
                if(indexOfLink>numOfCylinder-1)
                    return mat;
                for(int i=0; i<indexOfLink;i++){
                    mat=mat*data(i).MakeTrans();
                }
                return mat;

            }

            Eigen:: Matrix3f Viewer:: getParentsRotationMatrixes(int indexOfLink){
                if(indexOfLink==0 )
                    return Eigen:: Matrix3f::Identity();
                else {
                    Eigen::Matrix3f mat = data(1).Tout.rotation().matrix();
                    for (int i = 2; i < indexOfLink; i++) {
                        mat = mat * data(i).Tout.rotation().matrix();
                    }
                    return mat;
                }

            }
            double Viewer:: getYTheta(int index){
                Eigen:: Matrix3f rotMat=data(index).Tout.rotation().matrix();
                float check=rotMat.row(1)(1);
                if(check<1){
                    if(check>-1)
                        return atan2(rotMat.row(1)(0),-rotMat.row(1)(2));
                    else
                        return 0;
                }
                // (else )
                    return 0;

            }



            IGL_INLINE Eigen::Vector3f Viewer::getTip(int index) {
                Eigen::Vector3f ret = Eigen::Vector3f::Zero();
                for (int j = 0; j <index+1 ; j++) {
                    ret = ret+ getNewRotationVector(j);
                }
                return ret*1.6;
            }

            void Viewer::drawBox(Eigen::AlignedBox<double ,3> *box,int index, Eigen::RowVector3d color) {
                Eigen::Vector3d m=box->corner(box->BottomLeftFloor);
                Eigen::Vector3d M=box->corner(box->TopRightCeil);
                Eigen::MatrixXd V_box(8,3);
                V_box <<

                      m(0), m(1), m(2),
                        M(0), m(1), m(2),
                        M(0), M(1), m(2),
                        m(0), M(1), m(2),
                        m(0), m(1), M(2),
                        M(0), m(1), M(2),
                        M(0), M(1), M(2),
                        m(0), M(1), M(2);

                // Edges of the bounding box
                Eigen::MatrixXi E_box(12,2);
                E_box <<
                      0,1,
                        1,2,
                        2,3,
                        3,0,
                        4,5,
                        5,6,
                        6,7,
                        7,4,
                        0,4,
                        1,5,
                        2,6,
                        7,3;

                // Plot the corners of the bounding box as points
                data(index).add_points(V_box,color);


                // Plot the edges of the bounding box
                for (unsigned i=0;i<E_box.rows(); ++i) {
                    data(index).add_edges
                            (
                            V_box.row(E_box(i, 0)),
                            V_box.row(E_box(i, 1)),
//                                    Eigen::RowVector3d(0, 0, 0)
                            color
                    );
                }


            }

            Eigen::RowVector3f Viewer::getRandomLocation() {
                Eigen::RowVector3f location=Eigen::RowVector3f::Random();
                return location *10+Eigen::RowVector3f(0,2,0);
            }

            void Viewer::drawInGreen(int index) {
                Eigen::MatrixXd GREEN(1,3);
                GREEN(0,0) = 0;
                GREEN(0,1) = 1;
                GREEN(0,2) = 0;

                if(meshes.size()>index && !meshes[index].captured)
                    data(index).set_colors(GREEN);

            }
            void Viewer::drawInYellow(int index) {
                Eigen::MatrixXd YELLOW(1,3);
                YELLOW(0,0) = 255;
                YELLOW(0,1) = 255;
                YELLOW(0,2) = 0;

                if(meshes.size()>index && !meshes[index].captured)
                    data(index).set_colors(YELLOW);

            }



            void Viewer:: clearDataAndMeshes() {

                data_list.erase(data_list.begin() + 22, data_list.begin() + data_list.size());
                meshes.erase(meshes.begin() + 22, meshes.begin() + meshes.size());
                selected_data_index = 0;
                next_data_id=22;
                for (int j = numOfCylinder; j < meshes.size(); j++) {

                    if(meshes[j].pointsValue ==0)
                        meshes[j].pointsValue = 5;
                    meshes[j].captured = false;
                }

                for (int j = numOfCylinder; j < data_list.size(); j++)
                    data(j).set_visible(true);
            }

        void Viewer:: finishLevel(bool timePassed) {
//                    userPoints=0;
                    char answer;

                if(timePassed) {
                    std::cout << "TIME PASSED!(90 seconds) TYPE YOUR CHOOSE:" <<std::endl;
                    std::cout << "(B) - START A NEW GAME FROM THE BEGINNING" <<std::endl;
                    std::cout << "(Q) - QUIT" <<std::endl;
                    std::cin >>  answer;


                    if(answer=='B' ||answer=='b' )
                        init(1);


                    if(answer=='Q' ||answer=='q' )
                        std::exit(4713);

                }
                else{ // got enough points to finish the level

                    std::cout << "EXCELENT! YOU FINSHED LEVEL NUMBER " <<levelNumber<<". TYPE YOUR CHOOSE:" <<std::endl;
                    std::cout << "(N) - CONTINUE TO THE NEXT LEVEL" <<std::endl;
                    std::cout << "(B) - START A NEW GAME FROM THE BEGINNING" <<std::endl;
                    std::cout << "(Q) - QUIT" <<std::endl;
                    std::cin >>  answer;

                    if(answer=='N' ||answer=='n' )
                        init(levelNumber+1);

                    if(answer=='B' ||answer=='b' )
                        init(1);

                    if(answer=='Q' ||answer=='q' )
                        std::exit(4713);
                }


            }

            void Viewer::loadCheckCube() {
                load_mesh_from_file("/home/omer/CLionProjects/Final-Project-Snake/tutorial/data/cube.obj");
                Eigen::RowVector3f TR = -(this->MakeTrans()*this->getParentsTrans(this->numOfCylinder-1)*this->data(this->numOfCylinder-1).MakeTrans()).col(3).head(3); //A.col(1);
                Eigen::Vector3f a=  (this->getNewRotationVector(this->numOfCylinder)
                                     * -0.83);

                TR += a;

                data(data_list.size()-1).MyTranslate(TR ,true);
                this->meshes[data_list.size()-1].type="cube";
                this->meshes[data_list.size()-1].shouldMove =false;
            }


        } // end namespace
    } // end namespace
}

