#include "GameObject.h"

class ComponentBase
{
public:
	virtual void Init(GameObject *o);
	virtual void Update(float deltaTime);
	virtual void Render();
	virtual void Destroy();

protected:
	GameObject *owner;
};