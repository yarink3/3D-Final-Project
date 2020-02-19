#include "igl/opengl/glfw/renderer.h"

#include <GLFW/glfw3.h>
#include <igl/unproject_onto_mesh.h>
#include "igl/look_at.h"
#include <Eigen/Dense>
#include <math.h>
#include <chrono>


Renderer::Renderer() : selected_core_index(0),
                       next_core_id(2)
{
    core_list.emplace_back(igl::opengl::ViewerCore());
    core_list.front().id = 1;
    // C-style callbacks
    callback_init = nullptr;
    callback_pre_draw = nullptr;
    callback_post_draw = nullptr;
    callback_mouse_down = nullptr;
    callback_mouse_up = nullptr;
    callback_mouse_move = nullptr;
    callback_mouse_scroll = nullptr;
    callback_key_down = nullptr;
    callback_key_up = nullptr;

    callback_init_data = nullptr;
    callback_pre_draw_data = nullptr;
    callback_post_draw_data = nullptr;
    callback_mouse_down_data = nullptr;
    callback_mouse_up_data = nullptr;
    callback_mouse_move_data = nullptr;
    callback_mouse_scroll_data = nullptr;
    callback_key_down_data = nullptr;
    callback_key_up_data = nullptr;
    highdpi = 1;

    xold = 0;
    yold = 0;

}

IGL_INLINE void Renderer::draw( GLFWwindow* window)
{
    std::stringstream l1;
    l1 << "POINTS: "<< scn->userPoints;
    Eigen::Vector3d placeOfPoints = Eigen::Vector3d (-1,0,0);
    scn->data().add_label(placeOfPoints,l1.str());

    if(needToResetBecauseOfPoints==true){
        secondsPassed=0;
        levelfinishedBecauseScoreReached=false;
        needToResetBecauseOfPoints=false;
        scn->finishLevel(false);
    }

    if(levelfinishedBecauseScoreReached) {
        needToResetBecauseOfPoints = true;
        scn->start = time(0);

    }

    if(levelfinishedBecauseTimePassed==true) {
        secondsPassed=0;
        levelfinishedBecauseTimePassed=false;

        scn->finishLevel(true);

    }
    time_t start=scn->start;
    secondsPassed=difftime(time(0),start);
//    std::cout<<secondsPassed<<" seconds passed"<<std::endl;
    if(secondsPassed>90){
        if(shouldIK) {
            shouldIK = false;
            fix();
        }
        secondsPassed=0;
//        scn->clearDataAndMeshes();
        levelfinishedBecauseTimePassed=true;


    }
    using namespace std;
    using namespace Eigen;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    int width_window, height_window;
    glfwGetWindowSize(window, &width_window, &height_window);

    auto highdpi_tmp = (width_window == 0 || width == 0) ? highdpi : (width / width_window);

    if (fabs(highdpi_tmp - highdpi) > 1e-8)
    {
        post_resize(window,width, height);
        highdpi = highdpi_tmp;
    }

    for (auto& core : core_list)
    {
        core.clear_framebuffers();
    }

    for (auto& core : core_list)
    {
        int index=0;
        for (auto& mesh : scn->data_list)
        {
            if (mesh.is_visible & core.id)
            {
                core.draw(scn->MakeTrans()*scn->getParentsTrans(index),mesh,true);
//                core.draw(scn->MakeTrans(),mesh,true);
            }
            index++;
        }
    }
    for(int j=scn->numOfCylinder;j<scn->meshes.size();j++)
        if(scn->meshes[j].pointsValue<0)
            scn->drawInGreen(j);




}

void Renderer::SetScene(igl::opengl::glfw::Viewer* viewer)
{
    scn = viewer;
}

IGL_INLINE void Renderer::init(igl::opengl::glfw::Viewer* viewer)
{
    using namespace Eigen;
    scn = viewer;
    core().init();

    core().align_camera_center(scn->data().V, scn->data().F);
//    unsigned int left_view, right_view;
//    if(numOfCors >1){

        core().viewport = Eigen::Vector4f(0,0,500,800);
        left_view = core_list[0].id;
        right_view =
        append_core(Eigen::Vector4f(500,0,500,800));
        viewer->right_view = right_view;
        viewer->left_view = left_view;
         viewer->coreList = &core_list;
//        core_index(right_view-1);
    locateCore1();


        for (size_t i = 0; i <scn->data_list.size() ; i++) {

            core().toggle(scn->data(i).show_faces);

        }
//     }
}

void Renderer::UpdatePosition(double xpos, double ypos)
{
    xrel = xold - xpos;
    yrel = yold - ypos;
    xold = xpos;
    yold = ypos;
}

void Renderer::MouseProcessing(int button) {
    int index = scn->selected_data_index;

    if (button == 1)
        if (scn->meshPicked) {
            if (scn->meshes[index].type=="Ycylinder") {
                scn->data(0).MyTranslate(Eigen::Vector3f(-xrel / 2000.0f, 0, 0), true);
                scn->data(0).MyTranslate(Eigen::Vector3f(0, yrel / 2000.0f, 0), true);
                UpdateCore();
            }

        }

    else
        if (scn->meshPicked ) {
            if (scn->meshes[index].type=="Ycylinder") {
                scn->data().MyRotate(Eigen::Vector3f(0, 0, 1), yrel / 180.0f);
                scn->data().MyRotate(Eigen::Vector3f(1, 0, 0), xrel / 180.0f);
                fix();
                UpdateCore();

            }
        }
    }


Renderer::~Renderer()
{
    //if (scn)
    //	delete scn;
}

bool Renderer::Picking(double newx, double newy, float *pInt)
{
    int fid;
    //Eigen::MatrixXd C = Eigen::MatrixXd::Constant(scn->data().F.rows(), 3, 1);
    Eigen::Vector3f bc;
    double x = newx;
    double y = core().viewport(3) - newy;
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
    igl::look_at(core().camera_eye, core().camera_center, core().camera_up, view);
    view = view * (core().trackball_angle * Eigen::Scaling(core().camera_zoom * core().camera_base_zoom)
                   * Eigen::Translation3f(core().camera_translation + core().camera_base_translation)).matrix() * scn->MakeTrans()  *  scn->getParentsTrans(scn->selected_data_index) *  scn->data().MakeTrans();
    if (igl::unproject_onto_mesh(Eigen::Vector2f(x, y), view,
                                 core().proj, core().viewport, scn->data().V, scn->data().F, fid, bc))
    {
        Eigen::MatrixXi F = scn->data().F;
        Eigen::MatrixXd V = scn->data().V;

        //Vector4f v5 = Vector4f (1.0 f , 2.0 f , 3.0 f , 4.0 f ) ;

        Eigen::Vector3f v0,v1,v2,p;
        v0=V.row(F.row(fid)(0)).cast<float>();
        v1=V.row(F.row(fid)(1)).cast<float>();
        v2=V.row(F.row(fid)(2)).cast<float>();

        Eigen::Vector4f u0,u1,u2;
        u0=Eigen::Vector4f(v0[0] , v0[1], v0[2],1.0f);
        u1=Eigen::Vector4f(v1[0] , v1[1], v1[2],1.0f);
        u2=Eigen::Vector4f(v2[0] , v2[1], v2[2],1.0f);

        u0=view*u0;
        u1=view*u1;
        u2=view*u2;

        v0=Eigen::Vector3f(u0[0] , u0[1], u0[2]);
        v1=Eigen::Vector3f(u1[0] , u1[1], u1[2]);
        v2=Eigen::Vector3f(u2[0] , u2[1], u2[2]);

        p=v0*bc[0]+v1*bc[1]+v2*bc[2];
        *pInt=p[2];

        return true;
    }

    *pInt=-INFINITY;
    return false;

}

IGL_INLINE void Renderer::resize(GLFWwindow* window,int w, int h)
{
    if (window) {
        glfwSetWindowSize(window, w / highdpi, h / highdpi);
    }
    post_resize(window,w, h);
}

IGL_INLINE void Renderer::post_resize(GLFWwindow* window, int w, int h)
{
    if (core_list.size() == 1)
    {
        core().viewport = Eigen::Vector4f(0, 0, w, h);
    }
    else
    {
        // It is up to the user to define the behavior of the post_resize() function
        // when there are multiple viewports (through the `callback_post_resize` callback)
        core(left_view).viewport = Eigen::Vector4f(0, 0, w / 2, h);
       core(right_view).viewport = Eigen::Vector4f(w / 2, 0, (w / 2), h);


    }
//    for (unsigned int i = 0; i < plugins.size(); ++i)
//    {
//    	plugins[i]->post_resize(w, h);
//    }
    if (callback_post_resize)
    {

        callback_post_resize(window, w, h);
    }
}

IGL_INLINE igl::opengl::ViewerCore& Renderer::core(unsigned core_id /*= 0*/)
{
    assert(!core_list.empty() && "core_list should never be empty");
    int core_index;
    if (core_id == 0)
        core_index = selected_core_index;
    else
        core_index = this->core_index(core_id);
    assert((core_index >= 0 && core_index < core_list.size()) && "selected_core_index should be in bounds");
    return core_list[core_index];
}

IGL_INLINE const igl::opengl::ViewerCore& Renderer::core(unsigned core_id /*= 0*/) const
{
    assert(!core_list.empty() && "core_list should never be empty");
    int core_index;
    if (core_id == 0)
        core_index = selected_core_index;
    else
        core_index = this->core_index(core_id);
    assert((core_index >= 0 && core_index < core_list.size()) && "selected_core_index should be in bounds");
    return core_list[core_index];
}

IGL_INLINE bool Renderer::erase_core(const size_t index)
{
    assert((index >= 0 && index < core_list.size()) && "index should be in bounds");
    //assert(data_list.size() >= 1);
    if (core_list.size() == 1)
    {
        // Cannot remove last viewport
        return false;
    }
    core_list[index].shut(); // does nothing
    core_list.erase(core_list.begin() + index);
    if (selected_core_index >= index && selected_core_index > 0)
    {
        selected_core_index--;
    }
    return true;
}

IGL_INLINE size_t Renderer::core_index(const int id) const {
    for (size_t i = 0; i < core_list.size(); ++i)
    {
        if (core_list[i].id == id)
            return i;
    }
    return 0;
}

IGL_INLINE int Renderer::append_core(Eigen::Vector4f viewport, bool append_empty /*= false*/)
{
    core_list.push_back(core()); // copies the previous active core and only changes the viewport
    core_list.back().viewport = viewport;
    core_list.back().id = next_core_id;
    next_core_id <<= 1;
    if (!append_empty)
    {
        for (auto& data : scn->data_list)
        {

            data.set_visible(true, core_list.back().id);

            //data.copy_options(core(), core_list.back());
        }
    }
    selected_core_index = core_list.size() - 1;
    return core_list.back().id;
}

IGL_INLINE void Renderer::toFirstTip() {
    using namespace Eigen;
    Vector3f c0 = scn->getNewRotationVector(1);
    Vector3f c1 = scn->data(1).GetCenterOfRotation();
    Matrix3f m1 = scn->data(1).Tout.rotation().matrix();
    Matrix3f m2 = scn->getParentsRotationMatrixes(1);
    std::cout <<"m1  is\n " <<m1 <<std::endl;



    scn->data(1).Tout.prerotate(m2);



    std::cout <<"m111  is\n " <<scn->data(1+1).Tout.rotation().matrix() <<std::endl;



}


IGL_INLINE void Renderer::PrintSphereLocation() {
    using namespace Eigen;
    using namespace std;
    cout<< " size is :\n " <<scn->data_list.size() <<endl;

    Vector3f mt = getPicked();
    cout<< " the target is at is:\n " <<mt.transpose() <<endl;

}

Eigen::Vector3f Renderer::getPicked() {
        return scn->data().Tout.translation().matrix();

}

Eigen::Vector3f Renderer::getButtom() {

    return scn->data(0).Tout.translation().matrix();
}






IGL_INLINE void Renderer::PrintTip() {
    using namespace std;
    Eigen::Vector3f ret;
    ret = printTip(scn->numOfCylinder-1);

    cout<< "the tip is:\n " <<ret.transpose() <<endl;

}


IGL_INLINE Eigen::Vector3f Renderer::printTip(size_t index) {
    Eigen::Vector3f ret = Eigen::Vector3f::Zero();
    for (int j = 0; j <index+1 ; j++) {
        ret = ret+ scn->getNewRotationVector(j);
    }
    return ret*1.6;
}
IGL_INLINE void Renderer::RotateY(bool right) {
    using namespace Eigen;;
    using namespace std;
    if(!scn->meshPicked){
        //scn rotation
        if(right){  scn->MyRotate(Eigen::Vector3f(0, 1,0 ), 0.1);}
        else{   scn->MyRotate(Eigen::Vector3f(0, 1,0 ), -0.1);}
    }
    else {
        Matrix3f m = scn->data().Tout.rotation().matrix();
        m.transposeInPlace();

        if (right) {

            scn->data().MyRotate(m*Eigen::Vector3f (0,1,0),0.1);


        } else {

            scn->data().MyRotate(m*Eigen::Vector3f (0,1,0),-0.1);

        }

    }

}


IGL_INLINE void Renderer::RotateX(bool up) {
    using namespace Eigen;
    using namespace std;
    if(!scn->meshPicked){
        //scn rotation

        if(up){  scn->MyRotate(Eigen::Vector3f(1,0,0), 0.1);}
        else{   scn->MyRotate(Eigen::Vector3f(1,0,0), -0.1);}
    }
    else {

        if (up) {
            scn->data().MyRotate(Eigen::Vector3f(1,0,0), 0.1);
        } else {
            scn->data().MyRotate(Eigen::Vector3f(1,0,0), -0.1);
        }
    }

}

IGL_INLINE void Renderer::toggleIK() {
        if(getDistanceFromSphere()<=scn->numOfCylinder*1.6 && scn->meshes[scn->selected_data_index].captured==false) {
            shouldIK = !shouldIK;
            if (!shouldIK)
                fix();

        }

    else {
        if(scn->selected_data_index>14){
            scn->meshes[scn->selected_data_index].shouldMove=true;
            std::cout << "can not reach!" << std::endl;
          }
        }
}




double Renderer::getDistanceFromSphere(){
    Eigen::Vector3f e = getPicked() -/*( scn->getTip(scn->numOfCylinder-1)+ */ getButtom();
//    std::cout<<"The distance is:   "<<e.norm()<< std::endl;
    return e.norm();


}
IGL_INLINE void Renderer::animateIK() {

    using namespace Eigen;
    if(scn->selected_data_index < scn->numOfCylinder) {
        shouldIK = false;
        return;
    }

    Vector3f D=getPicked();
    Vector3f e = scn->getTip(scn->numOfCylinder-1)+getButtom();// get the tip of the last cylinder


    UpdateCore();

//    while (d >0.1 && shouldIK){ // TODO change 0.1 to scn->tol
    for(int i = scn->numOfCylinder-1;shouldIK  && i>-1 ; i--)
    {

        Vector3f e = scn->getTip(scn->numOfCylinder-1)+getButtom();// get the tip of the last cylinder
        Vector3f er;
        Vector3f rd;
        Vector3f r;
        if(i==0) {
            r = getButtom();
        } else{
            r=scn->getTip(i-1)+getButtom();
        }


        er=(e-r).normalized();
        rd=(D-r).normalized();


        if(isIntersected(&scn->meshes[scn->numOfCylinder-1].KDtree,&scn->meshes[scn->selected_data_index].KDtree)) {
            UpdateCore();

            shouldIK = false;
//            std::cout << scn->selected_data_index << std::endl;
            if (scn->meshes[scn->selected_data_index].type !="Ycylinder") {
                if(scn->meshes[scn->selected_data_index].pointsValue>0) scn->player.playHit();
                else scn->player.playBadHit();
                scn->userPoints=scn->userPoints+scn->meshes[scn->selected_data_index].pointsValue;
//                scn->data(scn->selected_data_index).clear();
                scn->data(scn->selected_data_index).set_visible(false,left_view);
                scn->data(scn->selected_data_index).set_visible(false,right_view);
                scn->meshes[scn->selected_data_index].captured=true;
                scn->meshes[scn->selected_data_index].pointsValue=0;

//                scn->meshes.erase(scn->meshes.begin()+scn->selected_data_index-2);
                std::cout <<"PLAYER POINTS SO FAR: "<< scn->userPoints << std::endl;
                if(scn->userPoints>25*(scn->levelNumber-1) && scn->userPoints%25==0) {
                    levelfinishedBecauseScoreReached = true;
//                    scn->clearDataAndMeshes();
                }
            }



            fix();
            return;
        }

        double alpha=acos(er.dot(rd));
        if(alpha>1)
            alpha=1;
        if(alpha<-1)
            alpha=-1;
        Vector3f axis=scn->getParentsRotationMatrixes(i).inverse()*er.cross(rd);

        double distance = getDistanceFromSphere();

        scn->data(i).MyRotate(axis.normalized() ,alpha/10 );

//        std::cout<< "distance is:  "<<distance <<std::endl;
//
//        std::cout<< "axis.normalized() is:  "<<axis.normalized() <<std::endl;
//        std::cout<< "alpha is:  "<<alpha <<std::endl;

//        scn->drawBox(&scn->meshes[9].KDtree.m_box,9,Eigen::RowVector3d(1,1,1));
//        scn->drawBox(&scn->meshes[scn->selected_data_index].KDtree.m_box,scn->selected_data_index,Eigen::RowVector3d(1,1,1));


        }
    }



//}

void Renderer::fix(){
    for(int i=0;i < scn->numOfCylinder;i++){
        double yTheta=scn->getYTheta(i);
//        mat = scn->getParentsTrans(i).block<3, 3>(0, 0).cast<double>() * mat;
        scn->data(i).MyRotate(Eigen::Vector3f (0,1,0),-yTheta);
        if(i!=scn->numOfCylinder-1) {
            Eigen::Matrix3f mat=scn->data(i+1).Tout.rotation().matrix();

            mat.transposeInPlace();

            scn->data(i+1).Tout.rotate(Eigen::AngleAxisf(yTheta,mat*Eigen::Vector3f (0,1,0)));

//            scn->data(i + 1).Tout.rotate(Eigen::AngleAxisf(-yTheta, scn->getNewRotationVector(i)));
        }
    }
}

IGL_INLINE void Renderer::printAngle() {
    using namespace std;
    if(!scn->meshPicked) {
        cout << "the scn  mat is\n "
             << (scn->MakeTrans()).block<3, 3>(0,0) <<endl;

    }
    else{
        if(scn->selected_data_index == 4){return;}
        cout<< "the matrix  of link num \n"<< scn->selected_data_index<<" is \n "<< (scn->getParentsTrans(scn->selected_data_index)*scn->data().MakeTrans()).block<3, 3>(0, 0) << endl;
    }


}

//void Renderer::collision() {
//    int index=scn->selected_data_index;
//    if(collisionHappend)
//    {
//        scn->data(9).clear();
//        scn->data().clear();
//        scn->data(9).set_mesh(scn->meshes[9].V,scn->meshes[9].F);
//        scn->data(1).set_mesh(scn->meshes[1].V,scn->meshes[1].F);
////        scn->drawBox(&scn->meshes[0].KDtree.m_box,0,Eigen::RowVector3d(1,0,0));
////        scn->drawBox(&scn->meshes[1].KDtree.m_box,1,Eigen::RowVector3d(1,0,0));
//        collisionHappend=false;
//    }
//
//    if(shouldColl)
//        scn->data().MyTranslate(vel, true);
//
//    else
//        collisionHappend=true;
//
//
//    shouldColl=!isIntersected(&scn->meshes[9].KDtree,&scn->meshes[scn->selected_data_index].KDtree);
//    collisionHappend=!shouldColl;
//
//}

void Renderer::toggleCol() {
    shouldColl = !shouldColl;
}

bool Renderer::isIntersected(igl::AABB<Eigen::MatrixXd, 3> *node1, igl::AABB<Eigen::MatrixXd, 3> *node2) {
    using namespace Eigen;

    Vector3d d1=node1->m_box.center();
    Vector3d d2=node2->m_box.center();

    Vector3f C1 = (scn->getParentsTrans(scn->numOfCylinder-1)*scn->data(scn->numOfCylinder-1).MakeTrans() * Eigen::Vector4f(d1[0],d1[1], d1[2], 1)).head(3);
    Vector3f C2 = (scn->data().MakeTrans() * Eigen::Vector4f(d2[0],d2[1], d2[2], 1)).head(3);
    Vector3f D=(C1-C2);


    Matrix3f A,B,C;


//     A = scn->data(0).Tout.rotation().matrix() * Eigen::Matrix3f::Identity();
    RowVector3f A0 = scn->getParentsRotationMatrixes(scn->numOfCylinder-1)*scn->data(scn->numOfCylinder-1).Tout.rotation().matrix() * Eigen::Vector3f(1,0,0); //A.col(1);
    RowVector3f A1 = scn->getParentsRotationMatrixes(scn->numOfCylinder-1)*scn->data(scn->numOfCylinder-1).Tout.rotation().matrix() * Eigen::Vector3f(0,1,0); //A.col(2);
    RowVector3f A2 = scn->getParentsRotationMatrixes(scn->numOfCylinder-1)*scn->data(scn->numOfCylinder-1).Tout.rotation().matrix() * Eigen::Vector3f(0,0,1); //A.col(0);
//    Eigen::Matrix3d A;
//    A << A0[0], A1[0], A2[0],
//            A0[1], A1[1], A2[1],
//            A0[2], A1[2], A2[2];

//    B = scn->data(1).Tout.rotation().matrix() * Eigen::Matrix3f::Identity();
    RowVector3f B0 = scn->data(scn->selected_data_index).Tout.rotation().matrix() * Eigen::Vector3f(1,0,0); //B.col(1);
    RowVector3f B1 = scn->data(scn->selected_data_index).Tout.rotation().matrix() * Eigen::Vector3f(0,1,0); //B.col(2);
    RowVector3f B2 = scn->data(scn->selected_data_index).Tout.rotation().matrix() * Eigen::Vector3f(0,0,1); //B.col(0);
//    Eigen::Matrix3d B;
//    B << B0[0], B1[0], B2[0],
//            B0[1], B1[1], B2[1],
//            B0[2], B1[2], B2[2];

    A .col(0)=A0;
    A .col(1)=A1;
    A .col(2)=A2;
    B .col(0)=B0;
    B .col(1)=B1;
    B .col(2)=B2;

    float a0 = node1->m_box.sizes().cast<float>()[0]/2;
    float a1 = node1->m_box.sizes().cast<float>()[1]/2;
    float a2 = node1->m_box.sizes().cast<float>()[2]/2;

    float b0 = node2->m_box.sizes().cast<float>()[0]/2;
    float b1 = node2->m_box.sizes().cast<float>()[1]/2;
    float b2 = node2->m_box.sizes().cast<float>()[2]/2;


    C=A.transpose()*B;
//    C=(scn->data(0).Tout.rotation().matrix().inverse()*scn->data(1).Tout.rotation().matrix());
    bool toCheck=arentIntersepted(A0,A1,A2,a0,a1,a2,B0,B1,B2,b0,b1,b2,C,D);

    if(toCheck)
        return false;

//if(!toCheck){
//        shouldColl=false;
//    }
    if(!toCheck){ // met
//        shouldColl=false;
        if(node1->is_leaf() ) {
            if (node2->is_leaf()) {
//                scn->drawBox(&node1->m_box,0,Eigen::RowVector3d(1,1,1));
//                scn->drawBox(&node2->m_box,9,Eigen::RowVector3d(1,1,1));
                std::cout << "intersected\n ";

                return true;
            }
            else{
                return isIntersected(node1,node2->m_right ) ||
                       isIntersected(node1,node2->m_left );
            }
        }
        else{ // node1 is not leaf
            if (node2->is_leaf()) {
                return isIntersected(node1->m_right,node2) ||
                       isIntersected(node1->m_left,node2 );
            }
            else{// both not leafs
                return isIntersected( node1->m_right,  node2->m_right) ||
                       isIntersected( node1->m_left,  node2->m_left) ||
                       isIntersected( node1->m_left,  node2->m_right) ||
                       isIntersected(node1->m_right,  node2->m_left);
            }
        }

    }
    else{
        return false;
    }

}

Eigen::Vector3d Renderer::getFaceNormal(Eigen::Vector3d a, Eigen::Vector3d b, Eigen::Vector3d c) {
    //aa is the "shared" point
    using namespace Eigen;
    Vector3d ab =a-b;
    Vector3d ac =a-c;
    return ab.cross(ac);
}

bool Renderer::arentIntersepted(Eigen::RowVector3f A0, Eigen::RowVector3f A1, Eigen::RowVector3f A2, float a0, float a1,
                                float a2, Eigen::RowVector3f B0, Eigen::RowVector3f B1, Eigen::RowVector3f B2, float b0,
                                float b1, float b2,Eigen::Matrix3f C, Eigen::Vector3f D) {


    float c00=(C.row(0)[0]),c01=(C.row(0)[1]),c02=(C.row(0)[2]),
            c10=C.row(1)[0],c11=(C.row(1)[1]),c12=(C.row(1)[2]),
            c20=(C.row(2)[0]),c21=(C.row(2)[1]),c22=(C.row(2)[2]);
    return
            a0+b0*abs(c00)+b1*abs(c01)+b2*abs(c02) < abs(A0.dot(D)) ||
            a1+b0*abs(c10)+b1*abs(c11)+b2*abs(c12) < abs(A1.dot(D)) ||
            a2+b0*abs(c20)+b1*abs(c21)+b2*abs(c22) < abs(A2.dot(D)) ||
            b0+a0*abs(c00)+a1*abs(c10)+a2*abs(c20) < abs(B0.dot(D)) ||
            b1+a0*abs(c01)+a1*abs(c11)+a2*abs(c21) < abs(B1.dot(D)) ||
            b2+a0*abs(c02)+a1*abs(c12)+a2*abs(c22) < abs(B2.dot(D)) ||

            a1*abs(c20)+a2*abs(c10)+b1*abs(c02)+b2*abs(c01) < abs(c10*A2.dot(D)-c20*A1.dot(D))||
            a1*abs(c21)+a2*abs(c11)+b0*abs(c02)+b2*abs(c00) < abs(c11*A2.dot(D)-c21*A1.dot(D))||
            a1*abs(c22)+a2*abs(c12)+b0*abs(c01)+b1*abs(c00) < abs(c12*A2.dot(D)-c22*A1.dot(D))||

            a0*abs(c20)+a2*abs(c00)+b1*abs(c12)+b2*abs(c11) < abs(c20*A0.dot(D)-c00*A2.dot(D))||
            a0*abs(c21)+a2*abs(c01)+b0*abs(c12)+b2*abs(c10) < abs(c21*A0.dot(D)-c01*A2.dot(D))||
            a0*abs(c22)+a2*abs(c02)+b0*abs(c11)+b1*abs(c10) < abs(c22*A0.dot(D)-c02*A2.dot(D))||

            a0*abs(c10)+a1*abs(c00)+b1*abs(c22)+b2*abs(c21) < abs(c00*A1.dot(D)-c10*A0.dot(D))||
            a0*abs(c11)+a2*abs(c01)+b0*abs(c22)+b2*abs(c20) < abs(c01*A1.dot(D)-c11*A0.dot(D))||
            a0*abs(c12)+a2*abs(c02)+b0*abs(c21)+b1*abs(c20) < abs(c02*A1.dot(D)-c12*A0.dot(D));

}

IGL_INLINE void Renderer::select_hovered_core(GLFWwindow* window)
{
    int width_window =500,
            height_window = 800;
    glfwGetFramebufferSize(window, &width_window, &height_window);
    for (int i = 0; i < core_list.size(); i++)
    {
        Eigen::Vector4f viewport = core_list[i].viewport;

        if ((xold > viewport[0]) &&
            (xold < viewport[0] + viewport[2]) &&
            ((height_window - yold) > viewport[1]) &&
            ((height_window - yold) < viewport[1] + viewport[3]))
        {
            selected_core_index = i;
            break;
        }
    }
}

void Renderer::moveSpheres() {

    for(int index=14; index < scn->data_list.size();index++  ){

        if(scn->meshes[index].movesInCurrentDirection==40){
            scn->meshes[index].movesInCurrentDirection=0;
            scn->meshes[index].changeDirections(scn->levelNumber);
        }
        if(scn->meshes[index].shouldMove==true) {
//            for(int k=0;k<36600;k++) {
            scn->data(index).MyTranslate(scn->meshes[index].vectorToMoveBy, true);
            scn->meshes[index].movesInCurrentDirection++;

//            }
//            scn->data(index).MyRotate(Eigen::Vector3f(0, 0, 1), 0.25);
//            scn->data(index).MyRotate(Eigen::Vector3f(1, 0, 0), 0.25);

        }
    }
//    goForward=!goForward;

}

void Renderer::locateCore1() {
    using namespace Eigen;
    RowVector3f N = scn->data(0).Tout.rotation().matrix() * Eigen::Vector3f(0,0,1);
    core(right_view).camera_up = N;
    RowVector3f E = scn->data(0).Tout.rotation().matrix() * Eigen::Vector3f(0,-1,0);
    core(right_view).camera_eye = E;

    RowVector3f TR = -scn->data(0).MakeTrans().col(3).head(3); //A.col(1);
    TR = TR + RowVector3f (0,-((0.91)+((scn->numOfCylinder-1)*1.6) ),0);
    core(right_view).camera_translation =TR;

    for (int i = 0; i<scn->numOfCylinder; ++i)
    {
//        scn->data(i).set_visible(true, left_view);
        scn->data(i).set_visible(false, right_view);
    }

}

void Renderer::UpdateCore() {

    Eigen::Matrix3f rota =  scn->getParentsRotationMatrixes(scn->numOfCylinder);
    // TODO - play with yTheta in order to fix the camera_eye
    core(right_view).camera_up = rota * Eigen::Vector3f(0,0,1);
    core(right_view).camera_eye = rota * Eigen::Vector3f(0,-1,0);
    Eigen::RowVector3f TR = -(scn->MakeTrans()*scn->getParentsTrans(scn->numOfCylinder-1)*scn->data(scn->numOfCylinder-1).MakeTrans()).col(3).head(3); //A.col(1);
    Eigen::Vector3f a=  (scn->getNewRotationVector(scn->numOfCylinder)
                         * -0.83);

    TR += a;

    core(right_view).camera_translation =TR;

}

//IGL_INLINE void Viewer::select_hovered_core()
//{
//	int width_window, height_window = 800;
//   glfwGetFramebufferSize(window, &width_window, &height_window);
//	for (int i = 0; i < core_list.size(); i++)
//	{
//		Eigen::Vector4f viewport = core_list[i].viewport;

//		if ((current_mouse_x > viewport[0]) &&
//			(current_mouse_x < viewport[0] + viewport[2]) &&
//			((height_window - current_mouse_y) > viewport[1]) &&
//			((height_window - current_mouse_y) < viewport[1] + viewport[3]))
//		{
//			selected_core_index = i;
//			break;
//		}
//	}
//}

//void Renderer::fabrik() {
//    using namespace std;
//    double target  = scn->data(scn->data_list.size()-1).getLocation; //get the location of the ball
//    vector<double> pos, gap;   //vector of positions
//
//    for (ViewerData  cyl: scn->data_list )
//    {
//        pos.push_back( cyl.location) ; //push the location of the cylyndar
//    }
//    int n = pos.size()-1;  //-1 ??
//    gap.resize(n-1);
//    for(int i =0 ;i<n-1 ; i++){
//        gap[i] = abs(pos[i+1] -pos[i] );
//    }
//
//    double dist = abs(pos[0] - target);
//    double sum = 0;
//    for( double x : pos){
//        sum =+ x;
//    }
//    if (dist >sum ){
//        //target is unreachable
//        vector<double> fromRoot , lamd;
//        lamd.resize(n-1);
//        fromRoot.resize(n-1);
//        for(int i =0 ;i<n-1 ; i++){
//            fromRoot[i] = (abs(target - pos[i]));
//            lamd[i] = (gap[i] /fromRoot[i] );
//            pos[i+1] = (1-lamd[i])*pos[i] +lamd[i]*target;
//        }
//
//
//    }
//
//    else{
//        //target is reachable
//        vector<double> fromRoot , lamd;
//        lamd.resize(n-1);
//        fromRoot.resize(n-1);
//        double b = pos[0];
//        double difA = abs(pos[n-1] - target);
//        while(difA>scn->tol) {
//            pos[n - 1] = target;
//            for (int i = n - 2; i >= 0; i--) {
//                fromRoot[i] = abs(pos[i + 1] - pos[i]);
//                lamd[i] = (gap[i] / fromRoot[i]);
//                pos[i + 1] = (1 - lamd[i]) * pos[i] + lamd[i] * target;
//
//            }
//
//            pos[0] = b;
//            for (int i = 0; i > n - 1; i++) {
//                fromRoot[i] = abs(pos[i + 1] - pos[i]);
//                lamd[i] = (gap[i] / fromRoot[i]);
//                pos[i + 1] = (1 - lamd[i]) * pos[i] + lamd[i] * target;
//
//            }
//            difA = abs(pos[pos.size() - 1] - target);
//            updatePosition(pos);
//
//        }
//    }
//}


//IGL_INLINE void Viewer::select_hovered_core()
//{
//	int width_window, height_window = 800;
//   glfwGetFramebufferSize(window, &width_window, &height_window);
//	for (int i = 0; i < core_list.size(); i++)
//	{
//		Eigen::Vector4f viewport = core_list[i].viewport;

//		if ((current_mouse_x > viewport[0]) &&
//			(current_mouse_x < viewport[0] + viewport[2]) &&
//			((height_window - current_mouse_y) > viewport[1]) &&
//			((height_window - current_mouse_y) < viewport[1] + viewport[3]))
//		{
//			selected_core_index = i;
//			break;
//		}
//	}
//}