// Copyright 2018 ...
#include "AugmentedRigidArm.h"

AugmentedRigidArm::AugmentedRigidArm(bool is_create_xacro) {
    rbdl_check_api_version(RBDL_API_VERSION);
    if (is_create_xacro) {
        create_xacro();
        return;
    }
    create_rbdl_model();
#if USE_ROS
    // ros::init() requires argc and argv for remapping, but since we're not using command line arguments for this, input placeholder values that won't be used.
    std::cout << "Setting up ROS node and publisher...\n";
    int tmp_c = 1;
    char *tmp_v[1];
    strcpy(tmp_v[0], "placeholder");
    ros::init(tmp_c, tmp_v, "joint_pub", ros::init_options::AnonymousName);
    ros::NodeHandle n;
    nodeHandle = &n;
    joint_pub = n.advertise<sensor_msgs::JointState>("/joint_states", 10);
    for (int i = 0; i < N_SEGMENTS; i++) {
        // set up joint names
        jointState.name.push_back(std::to_string(i) + "-base-ball-joint_x_joint");
        jointState.name.push_back(std::to_string(i) + "-base-ball-joint_y_joint");
        jointState.name.push_back(std::to_string(i) + "-base-ball-joint_z_joint");
        jointState.name.push_back(std::to_string(i) + "-a-b_joint");
        jointState.name.push_back(std::to_string(i) + "-middle-ball-joint_x_joint");
        jointState.name.push_back(std::to_string(i) + "-middle-ball-joint_y_joint");
        jointState.name.push_back(std::to_string(i) + "-middle-ball-joint_z_joint");
        jointState.name.push_back(std::to_string(i) + "-c-d_joint");
        jointState.name.push_back(std::to_string(i) + "-tip-ball-joint_x_joint");
        jointState.name.push_back(std::to_string(i) + "-tip-ball-joint_y_joint");
        jointState.name.push_back(std::to_string(i) + "-tip-ball-joint_z_joint");
    }
    for (int i = 0; i < N_SEGMENTS * JOINTS; i++)
        jointState.position.push_back(0.0);
    std::cout << "done.\n";
#endif
}


void AugmentedRigidArm::create_xacro() {
    std::cout << "generating XACRO file robot.urdf.xacro...\n";
    std::ofstream xacro_file;

    xacro_file.open("./urdf/robot.urdf.xacro");

    // write outlog text to the file
    xacro_file << "<?xml version='1.0'?><robot xmlns:xacro='http://www.ros.org/wiki/xacro' name='robot'>" <<
               "<xacro:include filename='macro_definitions.urdf.xacro' /><xacro:empty_link name='base_link'/>";

    // write out first PCC element
    // this is written outside for loop because parent of first PCC element must be called base_link
    xacro_file << "<xacro:PCC id='0' parent='base_link' child='mid-0' length='" << lengths[0] << "' mass='" << masses[0]
               << "'/>" <<
               "<xacro:empty_link name='mid-0'/>";
    // iterate over all the other PCC elements
    for (int i = 1; i < N_SEGMENTS; ++i) {
        xacro_file << "<xacro:PCC id='" << i << "' parent='" << "mid-" << i - 1 << "' child='" << "mid-" << i
                   << "' length='" << lengths[i] << "' mass='" << masses[i] << "'/>" <<
                   "<xacro:empty_link name='" << "mid-" << i << "'/>";
    }
    xacro_file << "</robot>";

    xacro_file.close();
    std::cout << "Finished generation. Run ./create_urdf in /urdf directory to generate robot.urdf from robot.urdf.xacro.(requires ROS)\n";
}

void AugmentedRigidArm::create_rbdl_model() {
    rbdl_model = new RigidBodyDynamics::Model();
    if (!RigidBodyDynamics::Addons::URDFReadFromFile("./urdf/robot.urdf", rbdl_model, false)) {
        std::cerr
                << "Error loading model ./urdf/robot.urdf. Make sure that robot.urdf.xacro is generated with ./create_xacro, and then converted to robot.urdf with ./create_urdf.sh (requires ROS)"
                << std::endl;
        abort();
    }
    int segments= rbdl_model->dof_count / JOINTS;
    std::cout << "Robot model created, with " << rbdl_model->dof_count << " DoF. It is a " << segments << "-segment arm.\n";
    if (segments != N_SEGMENTS){
        std::cout <<"Error: Number of segments in URDF does not match that in code. \n";
        abort();
    }
    rbdl_model->gravity = Vector3d(0., 0., -9.81);
}

Eigen::Matrix<double, 3, 1> AugmentedRigidArm::straw_bend_joint(double phi, double theta) {
    Eigen::Matrix<double, 3, 1> angles = Eigen::Matrix<double, 3, 1>::Zero();
    if (theta == 0)
        return angles;
    angles(0) = -atan(tan(theta) * sin(phi));
    angles(1) = asin(sin(theta) * cos(phi));
    angles(2) = -asin((cos(phi) * sin(phi) * cos(theta) - sin(phi) * cos(phi)) / cos(angles(1)));
    return angles;
}

void AugmentedRigidArm::update_m(Vector2Nd q) {
    double theta;
    double phi;
    double deltaL;
    for (int i = 0; i < N_SEGMENTS; ++i) {
        // use phi, theta parametrization because it's simpler for calculation
        if (q(2 * i) == 0) {
            if (q(2 * i + 1) == 0)
                phi = 0;
            else
                phi = PI / 2;
        } else
            phi = atan(q(2 * i + 1) / q(2 * i));
        if (q(2 * i) == 0)
            theta = -q(2 * i + 1) / (R_TRUNK * cos(PI / 2 - phi));
        else
            theta = -q(2 * i) / (R_TRUNK * cos(phi));
        if (theta < 0) {
            theta = -theta;
            phi += PI;
        }
        double b = lengths[i] / 2;
        if (theta != 0)
            b = lengths[i] / theta *
                sqrt(1.0 + 4.0 * sin(theta / 2.0) / theta * (sin(theta / 2.0) / theta - cos(theta / 2.0)));
        double nu = 0;
        if (theta != 0)
            nu = acos(1.0 / b * lengths[i] / theta * sin(theta / 2.0));

        deltaL = lengths[i] / 2.0 - b;
        m.block(JOINTS * i + 0, 0, 3, 1) = straw_bend_joint(phi, theta / 2 - nu);
        m(JOINTS * i + 3) = deltaL;
        m.block(JOINTS * i + 4, 0, 3, 1) = straw_bend_joint(phi, 2 * nu);
        m(JOINTS * i + 7) = deltaL;
        m.block(JOINTS * i + 8, 0, 3, 1) = m.block(JOINTS * i + 0, 0, 3, 1);
    }
}

void AugmentedRigidArm::update_Jm(Vector2Nd q) {
    // brute force calculate Jacobian numerically lol
    //todo: verify that this numerical method is actually okay
    // this particular implementation only works because m of each element is totally independent of other elements
    Vector2Nd q_deltaX = Vector2Nd(q);
    Vector2Nd q_deltaY = Vector2Nd(q);
    double epsilon = 0.0001;
    for (int i = 0; i < N_SEGMENTS; ++i) {
        q_deltaX(2 * i) += epsilon;
        q_deltaY(2 * i + 1) += epsilon;
    }
    update_m(q);
    Eigen::Matrix<double, JOINTS * N_SEGMENTS, 1> xi_current = Eigen::Matrix<double, JOINTS * N_SEGMENTS, 1>(m);
    update_m(q_deltaX);
    Eigen::Matrix<double, JOINTS * N_SEGMENTS, 1> xi_deltaX = Eigen::Matrix<double, JOINTS * N_SEGMENTS, 1>(m);
    update_m(q_deltaY);
    Eigen::Matrix<double, JOINTS * N_SEGMENTS, 1> xi_deltaY = Eigen::Matrix<double, JOINTS * N_SEGMENTS, 1>(m);
    for (int j = 0; j < N_SEGMENTS; ++j) {
        Jm.block(JOINTS * j, 2 * j + 0, JOINTS, 1) = (xi_deltaX - xi_current).block(JOINTS * j, 0, JOINTS, 1) / epsilon;
        Jm.block(JOINTS * j, 2 * j + 1, JOINTS, 1) = (xi_deltaY - xi_current).block(JOINTS * j, 0, JOINTS, 1) / epsilon;
    }
}

void AugmentedRigidArm::update_dJm(Vector2Nd q, Vector2Nd dq) {
    //todo: verify this numerical method too
    double epsilon = 0.1;
    Vector2Nd q_delta = Vector2Nd(q);
    q_delta += dq * epsilon;
    update_Jm(q);
    Eigen::Matrix<double, JOINTS * N_SEGMENTS, 2 * N_SEGMENTS> Jxi_current = Eigen::Matrix<double,
            JOINTS * N_SEGMENTS, 2 * N_SEGMENTS>(Jm);
    update_Jm(q_delta);
    Eigen::Matrix<double, JOINTS * N_SEGMENTS, 2 * N_SEGMENTS> Jxi_delta = Eigen::Matrix<double,
            JOINTS * N_SEGMENTS, 2 * N_SEGMENTS>(Jm);
    dJm = (Jxi_delta - Jxi_current) / epsilon;
}

void AugmentedRigidArm::update_Jxi(Vector2Nd q) {
    update_m(q);
    MatrixNd Jxi_6D = MatrixNd::Constant (6, rbdl_model->dof_count, 0.);
    Jxi_6D.setZero();
    CalcBodySpatialJacobian(*rbdl_model, m, JOINTS*N_SEGMENTS, Jxi_6D);
    Jxi = Jxi_6D.block(3,0,3,N_SEGMENTS*JOINTS);
}

void AugmentedRigidArm::update(Vector2Nd q, Vector2Nd dq) {

    // first update m (augmented model parameters)
    update_m(q);
    update_Jxi(q);
    extract_B_G();
    joint_publish();

    update_Jm(q);
    update_dJm(q, dq);

    update_m(q);

}

void AugmentedRigidArm::extract_B_G() {
    // the fun part- extracting the B_xi(inertia matrix) and G_xi(gravity) from RBDL, using unit vectors

    // first run Inverse Dynamics with dQ and ddQ as zero vectors (gives gravity vector)
    VectorNd dQ_zeros = VectorNd::Zero(N_SEGMENTS * JOINTS);
    VectorNd ddQ_zeros = VectorNd::Zero(N_SEGMENTS * JOINTS);
    VectorNd tau = VectorNd::Zero(N_SEGMENTS * JOINTS);
    InverseDynamics(*rbdl_model, m, dQ_zeros, ddQ_zeros, tau);
    G_xi = tau;

    // next, iterate through by making ddQ_zeros a unit vector and get inertia matrix
    for (int i = 0; i < N_SEGMENTS * JOINTS; ++i) {
        ddQ_zeros(i) = 0.1;
        InverseDynamics(*rbdl_model, m, dQ_zeros, ddQ_zeros, tau);
        B_xi.col(i) = (tau - G_xi) / ddQ_zeros(i);
        ddQ_zeros(i) = 0;
    }
}

AugmentedRigidArm::~AugmentedRigidArm() {
#if USE_ROS
    ros::shutdown();
#endif

}

void AugmentedRigidArm::joint_publish() {
#if USE_ROS
    jointState.header.stamp = ros::Time::now();
    for(int i=0; i<N_SEGMENTS*JOINTS; i++)
      jointState.position[i] = m(i);
    joint_pub.publish(jointState);
    //  std::cout<<"publishing"<<jointState<<"\n";
#endif
}
