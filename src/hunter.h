#pragma once


struct Hunter : public Entity {
	
    void Init(World* world) override;
	void Start(World* world) override;
	void Update(World* world) override;
	void Destroy(World* world) override;
};