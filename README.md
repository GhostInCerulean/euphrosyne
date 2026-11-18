![euphrosyne_alpha_edition.png](.github/euphrosyne_alpha_edition.png)

![License](https://img.shields.io/github/license/GhostInCerulean/Euphrosyne)
![Stars](https://img.shields.io/github/stars/GhostInCerulean/Euphrosyne)
![Last commit](https://img.shields.io/github/last-commit/GhostInCerulean/Euphrosyne)
![Issues](https://img.shields.io/github/issues/GhostInCerulean/Euphrosyne)
![Release](https://img.shields.io/github/v/release/GhostInCerulean/Euphrosyne)

# Description
Euphrosyne is a performant Vulkan and ECS based game engine for stylized games, using Blender as an editor, with in depth mod support.
Euphrosyne was thought out for solo devs / small teams that care about quality and performance. 

# Guide
A guide to walk you through the multiple stages of development is available in the [docs/guide](docs/guide) folder.

# Docs
Docs related to multiple engine components are available in the [docs](docs) folder.

# Community
TODO: Link to Armada community

# What does it looks like?
An introduction to what you will spend 99% of your time doing if you make a game with Euphrosyne.

## Writing gameplay code
Euphrosyne is ECS based, and uses the [flecs](https://www.flecs.dev/flecs/) library as it's ECS engine.
ECS might seem daunting at first but when you learn it and how to architecture code around it, it's quite powerful, intuitive and clean.

Here is an example system using the health we just defined.
````c++
struct HealthComponent 
{
    int value;
};

struct HealthModule 
{
    HealthModule(flecs::world& ecs) 
    {
        // 1. Declare your component to flecs, you don't have to be this verbose but
        // this gives the flecs explorer more info
        ecs.component<HealthComponent>()
           .member<int>("value")
        
        // 2. Declare the component as loadable from levels and prefabs
        g_properties.register("health", PropertyType::I32);
        
        // 3. Declare the system that kills dead people
        ecs.system<const Health>("Kill health <= 0 entities") // runs on entities with these components, given a descriptive name
           .kind(flecs::PostUpdate) // run after other systems
           .each([](flecs::entity e, const HealthComponent& health) 
           {
                if (health.value <= 0) 
                {
                    e.destruct();
                }    
           });
    }
};
````
And then, somewhere in your game's `ImportEcsModules` function:
```c++
ecs.import<HealthModule>();
```
Congrats, you've created a component and a system that runs on entities with that component!
Now how would we actually create entities that have that component?

## Making an asset
With Blender (or Bforartists) and preferrably a level editor plugin such as [Anvil](https://anvilleveldesign.com/), create your asset. In this example,
we are creating a level.
### 1. Model out your asset
I've done something quite simple here, if your game's levels looks like that, it might be time to hire a level designer.
If your model has animations, this is also when you write them.
![blender1.png](.github/blender1.png)

### 2. Add the data for the properties you created via custom properties
In the custom properties panel, create a new property, give it the name and type you want.
Here we give it the `Integer` type and `health` name as it is what expected our code that we wrote earlier.
![blender2.png](.github/blender2.png)

Now we have:

![blender3.png](.github/blender3.png)

### 3. Save the file in the right subfolder of your game's `assets` folder

Here, it is a level, so we will save it in `assets/levels`.

Congratulations, it will now be included in your game, and you will be able to load it.
