variantNames.push_back("gems");
variantLoaders.push_back([this]()
{
{
auto material = new SceneMaterial();material->name = "Light";
material->emission = glm::vec4(1.000000f, 0.980204f, 0.653571f, 2.600000f);
material->refractionIndex = glm::vec3(0.000000f, 0.000000f, 0.000000f);
material->absorption = glm::vec3(0.000000f, 0.000000f, 0.000000f);
materials.Add(SceneMaterial::Handle(1), material);
}
{
auto material = new SceneMaterial();material->name = "Material_2";
material->emission = glm::vec4(0.000000f, 0.000000f, 0.000000f, 0.000000f);
material->refractionIndex = glm::vec3(1.500000f, 1.550000f, 1.600000f);
material->absorption = glm::vec3(1.000000f, 1.000000f, 1.000000f);
materials.Add(SceneMaterial::Handle(2), material);
}
{
auto material = new SceneMaterial();material->name = "Material_3";
material->emission = glm::vec4(0.796556f, 1.000000f, 0.482143f, 0.020000f);
material->refractionIndex = glm::vec3(1.700000f, 1.750000f, 1.800000f);
material->absorption = glm::vec3(1.700000f, 0.000000f, 1.300000f);
materials.Add(SceneMaterial::Handle(3), material);
}
{
auto material = new SceneMaterial();material->name = "Material_4";
material->emission = glm::vec4(1.000000f, 0.389286f, 0.389286f, 0.030000f);
material->refractionIndex = glm::vec3(1.500000f, 1.550000f, 1.600000f);
material->absorption = glm::vec3(0.000000f, 36.632000f, 2.900000f);
materials.Add(SceneMaterial::Handle(4), material);
}
materials.nextFreeHandleValue = 5;
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(1.580939f, 1.210194f);
object->rotation = 0.000000f;
object->children.push_back(ISceneObject::Handle(2));
object->parent = ISceneObject::Handle(0);
objects.Add(ISceneObject::Handle(1), object);
}
{
auto object = new SceneObjectCircle();
object->radius = 0.193546f;
object->material = SceneMaterial::Handle(1);
object->parent = ISceneObject::Handle(1);
objects.Add(ISceneObject::Handle(2), object);
}
{
auto object = new SceneObjectPolygon();
object->rounding = 0.040000f;
object->material = SceneMaterial::Handle(2);
object->points.push_back(glm::vec2(-0.155267f, 0.334718f));
object->points.push_back(glm::vec2(0.193827f, -0.685631f));
object->points.push_back(glm::vec2(0.861091f, -0.319669f));
object->parent = ISceneObject::Handle(4);
objects.Add(ISceneObject::Handle(3), object);
}
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(0.782025f, 0.428174f);
object->rotation = -0.667488f;
object->children.push_back(ISceneObject::Handle(3));
object->parent = ISceneObject::Handle(0);
objects.Add(ISceneObject::Handle(4), object);
}
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(0.044475f, 0.174235f);
object->rotation = 0.157028f;
object->children.push_back(ISceneObject::Handle(6));
object->parent = ISceneObject::Handle(0);
objects.Add(ISceneObject::Handle(5), object);
}
{
auto object = new SceneObjectRectangle();
object->halfSize = glm::vec2(0.200877f, 0.578572f);
object->rounding = 0.090000f;
object->material = SceneMaterial::Handle(3);
object->parent = ISceneObject::Handle(5);
objects.Add(ISceneObject::Handle(6), object);
}
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(-0.694301f, -0.144578f);
object->rotation = 0.303016f;
object->children.push_back(ISceneObject::Handle(11));
object->parent = ISceneObject::Handle(0);
objects.Add(ISceneObject::Handle(10), object);
}
{
auto object = new SceneObjectPolygon();
object->rounding = 0.000000f;
object->material = SceneMaterial::Handle(4);
object->points.push_back(glm::vec2(0.369062f, 0.311215f));
object->points.push_back(glm::vec2(-0.168461f, 0.752455f));
object->points.push_back(glm::vec2(-0.445011f, 0.139244f));
object->points.push_back(glm::vec2(0.207052f, -0.694936f));
object->parent = ISceneObject::Handle(10);
objects.Add(ISceneObject::Handle(11), object);
}
objects.nextFreeHandleValue = 12;
rootObjects.push_back(ISceneObject::Handle(1));
rootObjects.push_back(ISceneObject::Handle(4));
rootObjects.push_back(ISceneObject::Handle(5));
rootObjects.push_back(ISceneObject::Handle(10));
if (GetRender()) RenderSetExposure(GetRender(), 2.800000f);
});
