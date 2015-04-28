
void CreateScene (NewtonWorld* world, SceneManager* sceneManager)
{
	Entity* floor;
	Entity* smilly;
	Entity* frowny;
	NewtonBody* floorBody;
	NewtonBody* smillyBody;
	NewtonBody* frownyBody;
	NewtonCollision* shape;



////////////////////////////////////////////////////////////////////////////////
// STATIC BODY 1 - FLOOR 
////////////////////////////////////////////////////////////////////////////////

	// Create a large body to be the floor
	floor = sceneManager->CreateEntity();
	floor->LoadMesh ("FloorBox.dat");
 
	// add static floor Physics
	shape = CreateNewtonBox (world, floor, 0);
	floorBody = CreateRigidBody (world, floor, shape, 0.0f);
	NewtonReleaseCollision (world, shape);
 
	// set the Transformation Matrix for this rigid body
	dMatrix matrix (floor->m_curRotation, floor->m_curPosition);
	NewtonBodySetMatrix (floorBody, &matrix[0][0]);
 
	// now we will use the properties of this body to set a proper world size.
	dVector minBox;
	dVector maxBox;
	NewtonCollisionCalculateAABB (shape, &matrix[0][0], &minBox[0], &maxBox[0]);
 
	// add some extra padding
	minBox.m_x -=  50.0f;
	minBox.m_y -= 500.0f;
	minBox.m_z -=  50.0f;
 
	maxBox.m_x +=  50.0f;
	maxBox.m_y += 500.0f;
	maxBox.m_z +=  50.0f;
 
	// set the new world size
	NewtonSetWorldSize (world, &minBox[0], &maxBox[0]);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////




 
////////////////////////////////////////////////////////////////////////////////
// DYNAMIC BODY 1
////////////////////////////////////////////////////////////////////////////////
 
	// add some visual entities.
	smilly = sceneManager->CreateEntity();
	smilly->LoadMesh ("Smilly.dat");
	smilly->m_curPosition.m_y = 10.0f;
	smilly->m_prevPosition = smilly->m_curPosition;
 
	// add a body with a box shape
	shape = CreateNewtonBox (world, smilly, 0);
	smillyBody = CreateRigidBody (world, smilly, shape, 10.0f);
	NewtonReleaseCollision (world, shape);
 
 
////////////////////////////////////////////////////////////////////////////////
// DYNAMIC BODY 2
////////////////////////////////////////////////////////////////////////////////
 
	// add some visual entities.
	frowny = sceneManager->CreateEntity();
	frowny->LoadMesh ("Frowny.dat");
	frowny->m_curPosition.m_z = 0.4f;
	frowny->m_curPosition.m_y = 10.0f + 0.4f;
	frowny->m_prevPosition = frowny->m_curPosition;
 
	// add a body with a Convex hull shape
	shape = CreateNewtonConvex (world, frowny, 0);
	frownyBody = CreateRigidBody (world, frowny, shape, 10.0f);
	NewtonReleaseCollision (world, shape);
}

/*

The first part of this function creates a large flat static body that will serve as the Floor where all other body will rest.

	floor = sceneManager->CreateEntity();
	floor->LoadMesh ("FloorBox.dat");
 
	// add static floor Physics
	shape = CreateNewtonBox (world, floor, 0);
	floorBody = CreateRigidBody (world, floor, shape, 0.0f);
	NewtonReleaseCollision (world, shape);

*/

NewtonCollision* CreateNewtonBox (NewtonWorld* world, Entity *ent, int shapeId)
{
	dVector minBox;
	dVector maxBox;
	NewtonCollision* collision;
 
	// Get the Bounding Box for this entity
	ent->GetBBox (minBox, maxBox);
 
	//calculate the box size and dimensions of the physics collision shape 
	dVector size (maxBox - minBox);
	dVector origin ((maxBox + minBox).Scale (0.5f));
	size.m_w = 1.0f;
	origin.m_w = 1.0f;
 
	// make and offset Matrix for this collision shape.
	dMatrix offset (GetIdentityMatrix());
	offset.m_posit = origin;
 
	// now create a collision Box for this entity
	collision = NewtonCreateBox (world, size.m_x, size.m_y, size.m_z, shapeId, &offset[0][0]);
 
	return collision;
}
/*
Basically it creates a collision box from the bounding box of a graphics entity, The box is centered at the geometrical center of the mesh entity, therefore you can have a mesh with a center at the lower bottom and the collision box will still be centered at the geometrical center. We encourage users to adopt this practice since it is more general and does not requires the user to change the origin of a mesh entity in the graphics editor.

Next line CreateRigidBody is also another helper function that creates general Rigid bodies. This function is found in the file RigidBodyUtil.cpp and it looks like this.
*/

NewtonBody* CreateRigidBody (NewtonWorld* world, Entity* ent, NewtonCollision* collision, dFloat mass)
{
	dVector minBox;
	dVector maxBox;
	dVector origin;
	dVector inertia;
	NewtonBody* body;
 
	// Now with the collision Shape we can crate a rigid body
	body = NewtonCreateBody (world, collision);
 
	// bodies can have a destructor. 
	// this is a function callback that can be used to destroy any local data stored 
	// and that need to be destroyed before the body is destroyed. 
	NewtonBodySetDestructorCallback (body, DestroyBodyCallback);
 
	// save the entity as the user data for this body
	nEWTONbODySetUserData (body, ent);
 
	// we need to set physics properties to this body
	dMatrix matrix (ent->m_curRotation, ent->m_curPosition);
	NewtonBodySetMatrix (body, &matrix[0][0]);
 
	// we need to set the proper center of mass and inertia matrix for this body
	// the inertia matrix calculated by this function does not include the mass.
	// therefore it needs to be multiplied by the mass of the body before it is used.
	NewtonConvexCollisionCalculateInertialMatrix (collision, &inertia[0], &origin[0]);	
 
	// set the body mass matrix
	NewtonBodySetMassMatrix (body, mass, mass * inertia.m_x, mass * inertia.m_y, mass * inertia.m_z);
 
	// set the body origin
	NewtonBodySetCentreOfMass (body, &origin[0]);
 
	// set the function callback to apply the external forces and torque to the body
	// the most common force is Gravity
	NewtonBodySetForceAndTorqueCallback (body, ApplyForceAndTorqueCallback);
 
	// set the function callback to set the transformation state of the graphic entity associated with this body 
	// each time the body change position and orientation in the physics world
	NewtonBodySetTransformCallback (body, SetTransformCallback);
 
	return body;
}

/*
This function creates a generic rigid body with some default values. The center of mass is set to the inertial center of mass of the collision shape. The function also set three functions callback: destructor, force and Torque and set transform.


The Destructor call back is a function that looks like this:
*/
void DestroyBodyCallback (const NewtonBody* body)
{
	// for now there is nothing to destroy
}

/*
This function is in charge to destroy any local data allocated by the application for the operation of this body, in these particular tutorials, the generic rigid body does not allocate any local data so the function DestroyBodyCallback does not do anything. The application may set this function to NULL, but it is good programming habit to assigned a basic destructor callback.


The Set Transform callback is a function that looks like this:

*/

// Transform callback to set the matrix of a the visual entity
void SetTransformCallback (const NewtonBody* body, const dFloat* matrix, int threadIndex)
{
	Entity* ent;
 
	// Get the position from the matrix
	dVector posit (matrix[12], matrix[13], matrix[14], 1.0f);
	dQuaternion rotation;
 
	// we will ignore the Rotation part of matrix and use the quaternion rotation stored in the body
	NewtonBodyGetRotation(body, &rotation.m_q0);
 
	// get the entity associated with this rigid body
	ent = (Entity*) NewtonBodyGetUserData(body);
 
	// since this tutorial run the physics and a different fps than the Graphics
	// we need to save the entity current transformation state before updating the new state.
	ent->m_prevPosition = ent->m_curPosition;
	ent->m_prevRotation = ent->m_curRotation;
 
	// set the new position and orientation for this entity
	ent->m_curPosition = posit;
	ent->m_curRotation = rotation;
}

/*

This function is called by the Newton engine every time the position and orientation of a rigid body changes. The arguments are the rigid body and the calculated Transformation matrix of the body. The transformation matrix is an array of 16 consecutive floats values representing a normal 4 x 4 transformation matrix. The application can safely cast the matrix pointer to any from a 4 x 4 matrix that conforms to the of D3D or OpenGl format.

In this particular implementation we because we run the physics at a fix time step the graphics, we need to interpolate the visual matrix from the current transformation step and the last Transformation Step. This is a trend that used by many games where the Graphic will be too slow if it run at the same fix step than the physics, or the physic will be too unstable if it is run at a variable time step.

For this tutorial the graphics entity contains the transform step for the previous position and rotation and the transformation step for the current step.

This function move the current state into the previous states and save the new Transform in to the current state. The only difference is that it does no uses the rotation part of transformation matrix, instead if get the Quaternion rotation that is already in the rigid body.


The next function callback is the apply force and torque callback

*/

// callback to apply external forces to body
void ApplyForceAndTorqueCallback (const NewtonBody* body, dFloat timestep, int threadIndex)
{
	dFloat Ixx;
	dFloat Iyy;
	dFloat Izz;
	dFloat mass;
 
	// for this tutorial the only external force in the Gravity
	NewtonBodyGetMassMatrix (body, &mass, &Ixx, &Iyy, &Izz);
 
	dVector gravityForce  (0.0f, mass * GRAVITY, 0.0f, 1.0f);
	NewtonBodySetForce(body, &gravityForce[0]);
}

/*

This function is call for each dynamics body in the scene even if they are rest sleeping. The job of this function is to apply all of external forces and torque to the rigid body. In this case since this is a default behavior, this function it only apply the standard gravity force.

One thing worth mention is that in the Newton engine forces and torques are applied to the center of mass of the body by definition. This means that if a force is applied to a body at a point other that the center of gravity the application most also apply the torque penetrate by that force on the body.


An example of a helper function to calculate the torque generate by a force of the center of mass of the body looks like this:

*/

dVector CalculateToqueAtPoint (const NewtonBody* body, const dVector& point, const dVector& force)
{
	dVector com;
	dMatrix matrix;
 
	NewtonBodyGetMatrix (body, &matrix[0][0]);
	NewtonBodyGetCentreOfMass (body, &matrix[0][0]);
	com = matrix.TransformVector (com);
	return (point - com) *  force;
}

/*
Basically is use the definition of torque generated by a force, which is the cross product of the distance from the point of action of the force to the center of mass of the body and the force vector.


After function CreateRigidBody returns to function CreateScene, it releases the Collision shape by calling NewtonReleaseCollision. This is a very important step for clean coding that does not leaves memory leaks after you destroy the rigid body and the Newton world. The reason the function call Release instead of destroy is that in the Newton engine Collision shape can be shared with other rigid bodies, for example you could place the same collision shape on other places by creating more rigid bodies passing the same collision shape. Each time a new rigid body is created the engine it increases the collision shape reference count. The application can check the number of references a collision shape has by calling GetCollisionInfo on the shape.


The next lines of function CreateScene make sure that the static body matrix is place correctly and also resize the world
*/
	// set the Transformation Matrix for this rigid body
	dMatrix matrix (floor->m_curRotation, floor->m_curPosition);
	NewtonBodySetMatrix (floorBody, &matrix[0][0]);
 
	// now we will use the properties of this body to set a proper world size.
	dVector minBox;
	dVector maxBox;
	NewtonCollisionCalculateAABB (shape, &matrix[0][0], &minBox[0], &maxBox[0]);
 
	// add some extra padding
	minBox.m_x -=  50.0f;
	minBox.m_y -= 500.0f;
	minBox.m_z -=  50.0f;
 
	maxBox.m_x +=  50.0f;
	maxBox.m_y += 500.0f;
	maxBox.m_z +=  50.0f;
 
	// set the new world size
	NewtonSetWorldSize (world, &minBox[0], &maxBox[0]);

/*
The first line set the transformation matrix of the entity. This is necessary because as you recall the Netwon only call the function transform call back of a rigid body when the body change position, but static bodies never change position, therefore the do not get a Transform callback call. This is important because some time I had seen applications where the world and the static mesh are out of place and it looks like it is a bug, when in reality it is an initialization problem. In fact it is good practice to always set the initial transformation matrix of any kind of entity.

The second part uses the bounding Box of the collision mesh to set the final size of the physics world. This is also an important step because in the Newton world the size of the world is finite. The consequence of not doing this is that when a body move to a position the is outside the default world size the physics silently stop working on that body.


The final line of function CreateScene, just repeat the same process of adding more physics bodies to the scene

*/

	// add some visual entities.
	smilly = sceneManager->CreateEntity();
	smilly->LoadMesh ("Smilly.dat");
	smilly->m_curPosition.m_y = 10.0f;
	smilly->m_prevPosition = smilly->m_curPosition;
 
	// add a body with a box shape
	shape = CreateNewtonBox (world, smilly, 0);
	smillyBody = CreateRigidBody (world, smilly, shape, 10.0f);
	NewtonReleaseCollision (world, shape);
 
 
	// add some visual entities.
	frowny = sceneManager->CreateEntity();
	frowny->LoadMesh ("Frowny.dat");
	frowny->m_curPosition.m_z = 0.4f;
	frowny->m_curPosition.m_y = 10.0f + 0.4f;
	frowny->m_prevPosition = frowny->m_curPosition;
 
	// add a body with a Convex hull shape
	shape = CreateNewtonConvex (world, frowny, 0);
	frownyBody = CreateRigidBody (world, frowny, shape, 10.0f);
	NewtonReleaseCollision (world, shape);
/*

The only difference between the creation of these bodies is that these bodies
have non zero mass. In the Newton world when a body have a zero Mass, it is
consider to be a Static body. Also these are much more smaller entities.

*/

The second body uses a different collision mesh called Convex Hull. The utility function to create a convex hull collision mesh looks like this

NewtonCollision* CreateNewtonConvex (NewtonWorld* world, Entity *ent, int shapeId)
{
	// now create a convex hull shape from the vertex geometry 
	return NewtonCreateConvexHull(world, ent->m_vertexCount, ent->m_vertex, 3 * sizeof (dFloat), 0.1f, shapeId, NULL);
}

/*

This is a very simple function that just passes the parameters to the Newton function that creates a convex hull shape form a clud of points. This function is different from CreateBox, in the is does not calculate an offset matrix for centering the convex mesh. The reason why this works is that in general entities are made in such way the the points are around the origin, for example a 1 x 1 x 1 box will have points between -0.5 and 0.5 around the origin. When this is the case the transform offset is not really necessary since the convex can work with these points as they are. However let us say the box was placed somewhere far away from the origin, in the situation the convex hull still works, but it will produce a collision mesh that will have its points also far away from the origin and this will create lots of extra unnecessary works for the engine, plus it is also possible that the collision system may malfunction because lost of accuracy in floating point values. A more general function will be something like this

*/

NewtonCollision* CreateNewtonConvex (NewtonWorld* world, Entity *ent, int shapeId)
{
	dVector minBox;
	dVector maxBox;
	NewtonCollision* collision;
	dVector* tmpArray = new dVector [ent->m_vertexCount];
 
	// Get the Bounding Box for this entity
	ent->GetBBox (minBox, maxBox);
 
	dVector size (maxBox - minBox);
	dVector origin ((maxBox + minBox).Scale (0.5f));
	size.m_w = 1.0f;
	origin.m_w = 1.0f;
 
	// Translate al the points to the origin point
	for (int i = 0; i < ent->m_vertexCount; i ++) {
		dVector tmp (ent->m_vertex[i * 3 + 0], ent->m_vertex[i * 3 + 1], ent->m_vertex[i * 3 + 2], 0.0f);
		tmpArray[i] = tmp - origin;
	}
 
	// make and offset Matrix for this collision shape.
	dMatrix offset (GetIdentityMatrix());
	offset.m_posit = origin;
 
	// now create a convex hull shape from the vertex geometry 
	collision = NewtonCreateConvexHull(world, ent->m_vertexCount, &tmpArray[0][0], sizeof (dVector), 0.1f, shapeId, &offset[0][0]);
 
	delete tmpArray;
 
	return collision;
}

/*

You can clearly see why the special case Convex hull is chosen foe these tutorial. Mainly because we know we are not building convex hulls for entities that are create in global space and has it points far away from the origin, secondly because the special case is much simpler and faster since it does not requires extra space and extra calculation of the origin,

*/
