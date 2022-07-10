variantNames.push_back("refraction");
variantLoaders.push_back([this]()
{
{
auto material = new SceneMaterial();material->name = "Material_1";
material->emission = glm::vec4(1.000000f, 1.000000f, 1.000000f, 1.000000f);
material->refractionIndex = glm::vec3(0.000000f, 0.000000f, 0.000000f);
material->absorption = glm::vec3(0.000000f, 0.000000f, 0.000000f);
materials.Add(SceneMaterial::Handle(1), material);
}
{
auto material = new SceneMaterial();material->name = "Material_2";
material->emission = glm::vec4(1.000000f, 1.000000f, 1.000000f, 0.000000f);
material->refractionIndex = glm::vec3(1.800000f, 1.800000f, 1.800000f);
material->absorption = glm::vec3(0.000000f, 0.000000f, 0.000000f);
materials.Add(SceneMaterial::Handle(2), material);
}
{
auto material = new SceneMaterial();material->name = "Material_3";
material->emission = glm::vec4(0.000000f, 0.000000f, 0.000000f, 0.000000f);
material->refractionIndex = glm::vec3(1.650000f, 1.750000f, 1.850000f);
material->absorption = glm::vec3(0.000000f, 0.000000f, 0.000000f);
materials.Add(SceneMaterial::Handle(3), material);
}
materials.nextFreeHandleValue = 4;
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(-1.504749f, 0.025950f);
object->rotation = 0.000000f;
object->children.push_back(ISceneObject::Handle(3));
object->parent = ISceneObject::Handle(0);
objects.Add(ISceneObject::Handle(1), object);
}
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(-0.804263f, 0.014829f);
object->rotation = 0.000000f;
object->children.push_back(ISceneObject::Handle(4));
object->parent = ISceneObject::Handle(7);
objects.Add(ISceneObject::Handle(2), object);
}
{
auto object = new SceneObjectCircle();
object->radius = 0.368821f;
object->material = SceneMaterial::Handle(1);
object->parent = ISceneObject::Handle(1);
objects.Add(ISceneObject::Handle(3), object);
}
{
auto object = new SceneObjectCircle();
object->radius = 0.363266f;
object->material = SceneMaterial::Handle(2);
object->parent = ISceneObject::Handle(2);
objects.Add(ISceneObject::Handle(4), object);
}
{
auto object = new SceneObjectRectangle();
object->halfSize = glm::vec2(0.214815f, 0.935185f);
object->rounding = 0.000000f;
object->material = SceneMaterial::Handle(2);
object->parent = ISceneObject::Handle(6);
objects.Add(ISceneObject::Handle(5), object);
}
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(-1.021080f, -0.326228f);
object->rotation = 0.000000f;
object->children.push_back(ISceneObject::Handle(5));
object->parent = ISceneObject::Handle(7);
objects.Add(ISceneObject::Handle(6), object);
}
{
auto object = new SceneObjectDifference();
object->children.push_back(ISceneObject::Handle(2));
object->children.push_back(ISceneObject::Handle(6));
object->parent = ISceneObject::Handle(0);
objects.Add(ISceneObject::Handle(7), object);
}
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(-0.240908f, 0.294717f);
object->rotation = 0.000000f;
object->children.push_back(ISceneObject::Handle(9));
object->parent = ISceneObject::Handle(0);
objects.Add(ISceneObject::Handle(8), object);
}
{
auto object = new SceneObjectPolygon();
object->rounding = 0.000000f;
object->material = SceneMaterial::Handle(3);
object->points.push_back(glm::vec2(0.320419f, 0.491196f));
object->points.push_back(glm::vec2(0.776466f, -1.006423f));
object->points.push_back(glm::vec2(-0.018149f, -1.005152f));
object->parent = ISceneObject::Handle(8);
objects.Add(ISceneObject::Handle(9), object);
}
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(-1.189597f, -0.441149f);
object->rotation = 0.000000f;
object->children.push_back(ISceneObject::Handle(11));
object->parent = ISceneObject::Handle(12);
objects.Add(ISceneObject::Handle(10), object);
}
{
auto object = new SceneObjectRectangle();
object->halfSize = glm::vec2(0.190741f, 1.585185f);
object->rounding = 0.000000f;
object->material = SceneMaterial::Handle(0);
object->parent = ISceneObject::Handle(10);
objects.Add(ISceneObject::Handle(11), object);
}
{
auto object = new SceneObjectDifference();
object->children.push_back(ISceneObject::Handle(10));
object->children.push_back(ISceneObject::Handle(13));
object->parent = ISceneObject::Handle(0);
objects.Add(ISceneObject::Handle(12), object);
}
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(-1.360205f, 0.025950f);
object->rotation = 0.000000f;
object->children.push_back(ISceneObject::Handle(14));
object->parent = ISceneObject::Handle(12);
objects.Add(ISceneObject::Handle(13), object);
}
{
auto object = new SceneObjectCircle();
object->radius = 0.368993f;
object->material = SceneMaterial::Handle(0);
object->parent = ISceneObject::Handle(13);
objects.Add(ISceneObject::Handle(14), object);
}
objects.nextFreeHandleValue = 15;
rootObjects.push_back(ISceneObject::Handle(1));
rootObjects.push_back(ISceneObject::Handle(7));
rootObjects.push_back(ISceneObject::Handle(8));
rootObjects.push_back(ISceneObject::Handle(12));
if (GetRender()) RenderSetExposure(GetRender(), 8.000000f);
});
