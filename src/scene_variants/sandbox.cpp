variantNames.push_back("sandbox");
variantLoaders.push_back([this]()
{
{
auto material = new SceneMaterial();material->name = "Light";
material->emission = glm::vec4(1.000000f, 1.000000f, 1.000000f, 1.730000f);
material->refractionIndex = glm::vec3(0.000000f, 0.000000f, 0.000000f);
material->absorption = glm::vec3(0.000000f, 0.000000f, 0.000000f);
materials.Add(SceneMaterial::Handle(1), material);
}
{
auto material = new SceneMaterial();material->name = "Material_2";
material->emission = glm::vec4(0.000000f, 0.000000f, 0.000000f, 0.000000f);
material->refractionIndex = glm::vec3(1.631000f, 1.412000f, 1.330000f);
material->absorption = glm::vec3(0.000000f, 0.000000f, 0.000000f);
materials.Add(SceneMaterial::Handle(2), material);
}
materials.nextFreeHandleValue = 3;
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(0.796850f, 0.544949f);
object->rotation = 0.000000f;
object->children.push_back(ISceneObject::Handle(2));
object->parent = ISceneObject::Handle(0);
objects.Add(ISceneObject::Handle(1), object);
}
{
auto object = new SceneObjectCircle();
object->radius = 0.193547f;
object->material = SceneMaterial::Handle(1);
object->parent = ISceneObject::Handle(1);
objects.Add(ISceneObject::Handle(2), object);
}
{
auto object = new SceneObjectPolygon();
object->rounding = 0.050000f;
object->material = SceneMaterial::Handle(2);
object->points.push_back(glm::vec2(-0.144802f, 0.300485f));
object->points.push_back(glm::vec2(-0.265706f, -0.386996f));
object->points.push_back(glm::vec2(0.468613f, -0.034264f));
object->parent = ISceneObject::Handle(4);
objects.Add(ISceneObject::Handle(3), object);
}
{
auto object = new SceneObjectTransform();
object->translation = glm::vec2(0.105629f, 0.061168f);
object->rotation = -0.667488f;
object->children.push_back(ISceneObject::Handle(3));
object->parent = ISceneObject::Handle(0);
objects.Add(ISceneObject::Handle(4), object);
}
objects.nextFreeHandleValue = 5;
rootObjects.push_back(ISceneObject::Handle(1));rootObjects.push_back(ISceneObject::Handle(4));if (GetRender()) RenderSetExposure(GetRender(), 2.062000f);
});
