# Modding SDK Implementation Plan

Plan pour livrer le SDK defini dans `docs/modding-sdk.md`: mods C++ natifs charges depuis Settings, API publique basee sur `flecs::world`, et bridge Dusklight minimal mais extensible.

## 0. Etat Actuel

Deja fait:

- Flecs est vendore dans `libs/flecs/flecs.c` et `libs/flecs/flecs.h`.
- `CMakeLists.txt` cree `add_library(flecs STATIC ...)` pres de `libs/freeverb`.
- `dusklight` link deja `flecs`.
- `nlohmann/json` est deja disponible via FetchContent.

A ne pas refaire: wrapper type `dusk::World`. Le code mod recoit directement `flecs::world*`.

## 1. Arborescence Cible

Ajouter ces fichiers:

```txt
include/dusklight/sdk.hpp
include/dusklight/modding/components.hpp
include/dusklight/modding/events.hpp
include/dusklight/modding/version.hpp

src/dusk/modding/sdk_world.hpp/.cpp
src/dusk/modding/mod_manifest.hpp/.cpp
src/dusk/modding/shared_library.hpp/.cpp
src/dusk/modding/mod_manager.hpp/.cpp
src/dusk/modding/game_bridge.hpp/.cpp
```

Ajouter toutes les sources dans `files.cmake`, dans le bloc `src/dusk/...` vers `src/dusk/logging.cpp` / `src/dusk/settings.cpp`. Les headers publics `include/dusklight/...` peuvent etre inclus par les mods externes; les headers `src/dusk/modding/...` restent internes au runtime.

## 2. API Publique SDK

`include/dusklight/sdk.hpp` doit etre le seul include recommande pour les mods:

```cpp
#include <flecs.h>
#include <dusklight/modding/components.hpp>
#include <dusklight/modding/events.hpp>
#include <dusklight/modding/version.hpp>

#define DUSK_MOD_EXPORT ...
using DuskModInitFn = void (*)(flecs::world*);
#define DUSK_MOD_INIT(name) extern "C" DUSK_MOD_EXPORT void dusk_mod_init(flecs::world* name)
```

Export par plateforme:

- Windows: `__declspec(dllexport)`
- Linux/macOS: `__attribute__((visibility("default")))`

Composants v0:

- `dusklight::Transform { float x, y, z; short pitch, yaw, roll; }`
- `dusklight::Velocity { float x, y, z; }`
- `dusklight::NativeActor { uint32_t id; void* ptr; }` dans un bloc clairement instable
- tags: `Player`, `Enemy`, `Npc`, `Item`, `Scene`, `Room`

Events v0:

- `dusklight::events::Frame { float delta_seconds; uint64_t frame_index; }`
- `dusklight::events::ActorCreated { uint32_t id; }`
- `dusklight::events::ActorDestroyed { uint32_t id; }`
- `SceneLoaded`, `RoomLoaded` plus tard, quand les transitions scene/room sont cartographiees proprement.

Politique v0: `Transform` est read-only; les mods lisent l'etat miroir, le moteur n'applique pas encore les ecritures.

## 3. Monde Flecs Runtime

Implementer dans `src/dusk/modding/sdk_world.*`:

```cpp
namespace dusk::modding {
flecs::world& world();
void initialize();
void shutdown();
void progress(float deltaSeconds);
uint64_t frame_index();
}
```

Implementation:

- garder un `std::unique_ptr<flecs::world>` statique;
- `initialize()` cree le monde une seule fois et enregistre les composants/tags publics;
- `progress()` emet `events::Frame`, appelle `world.progress(deltaSeconds)`, puis incremente le frame index;
- `shutdown()` detruit le monde apres unload des mods.

Points d'appel dans `src/m_Do/m_Do_main.cpp`:

- include `dusk/modding/sdk_world.hpp` et `dusk/modding/mod_manager.hpp`;
- appeler `dusk::modding::initialize()` apres init config/logging et avant que Settings puisse charger un mod;
- dans `main01()`, apres `fopAcM_initManager()` appeler le setup bridge;
- dans la boucle principale, appeler `game_bridge::sync_before_mods()` puis `modding::progress(dt)` apres la simulation (`fapGm_Execute()`/audio) et avant `aurora_end_frame()`;
- appeler `modding::shutdown()` juste avant `dusk::ui::shutdown()` a la sortie.

Pour le cas interpolation, commencer simple: ne progresser Flecs qu'une fois par frame presentee, pas par sous-tick. Les sous-ticks pourront venir apres.

## 4. Loader `.so` / `.dll` / `.dylib`

Implementer `src/dusk/modding/shared_library.*`:

```cpp
class SharedLibrary {
public:
    bool open(const std::filesystem::path& path, std::string* error);
    void* symbol(const char* name, std::string* error) const;
    void close();
    bool is_open() const;
};
```

Backends:

- `_WIN32`: `LoadLibraryW`, `GetProcAddress`, `FreeLibrary`
- autres desktop: `dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL)`, `dlsym`, `dlclose`

CMake: sur Linux, verifier si `${CMAKE_DL_LIBS}` est deja dans une dependance. Sinon ajouter a `target_link_libraries(dusklight PRIVATE ... ${CMAKE_DL_LIBS})`.

ABI Flecs:

- le mod recoit un `flecs::world*` cree par Dusklight, donc il ne doit pas embarquer son propre runtime Flecs separe;
- exporter les symboles de l'executable pour que les `.so` resolvent les appels Flecs vers Dusklight (`set_target_properties(dusklight PROPERTIES ENABLE_EXPORTS ON)` ou link option equivalente selon plateforme);
- l'exemple `hello_flecs` inclut les headers SDK/Flecs mais ne link pas une deuxieme copie de `flecs.c`;
- verifier avec `ldd -r` / chargement Settings que les symboles Flecs sont resolus avant d'appeler `dusk_mod_init`.

Contraintes:

- ne pas unload/reload chaud en v0 si le mod a enregistre des systems Flecs; marquer "reload requires restart";
- garder la lib ouverte jusqu'au shutdown;
- toutes les erreurs remontent en string et loguent via `aurora::Module Log{"dusk::modding"}`.

## 5. Manifest Et Resolution De Chemin

`mod_manifest.*` parse `mod.json` via `io::FileStream::ReadAllBytes()` + `nlohmann::json`.

Format:

```json
{
  "id": "hello_flecs",
  "name": "Hello Flecs",
  "version": "0.1.0",
  "sdk_version": "0.1.0",
  "library": "hello_flecs"
}
```

Validation:

- `id`, `name`, `version`, `sdk_version`, `library` requis;
- `id`: `[a-z0-9_]+`;
- `library`: basename seulement, pas `/`, `\`, `..`;
- `sdk_version`: compatible avec `DUSKLIGHT_SDK_VERSION_MAJOR/MINOR`;
- path final: dossier du manifest + nom lib + extension plateforme.

Fonctions:

```cpp
std::filesystem::path mods_directory(); // data::configured_data_path() / "mods"
std::optional<ModManifest> load_manifest(path, std::string* error);
std::filesystem::path resolve_library_path(const ModManifest&, const path& modRoot);
```

Creer le dossier `mods` si l'utilisateur ouvre/charge depuis l'UI.

## 6. Mod Manager

`mod_manager.*` est le point d'acces pour UI + runtime:

```cpp
struct LoadedMod {
    ModManifest manifest;
    std::filesystem::path root;
    std::filesystem::path library_path;
    SharedLibrary library;
    bool initialized = false;
    std::string status;
};

bool load_mod_manifest(const std::filesystem::path& manifestPath, std::string* error);
bool load_mod_library(const std::filesystem::path& libraryPath, std::string* error);
std::span<const LoadedMod> loaded_mods();
std::string last_error();
void shutdown();
```

Flow `load_mod_manifest`:

1. parser `mod.json`;
2. refuser un `id` deja charge;
3. resoudre lib;
4. ouvrir lib;
5. resoudre `dusk_mod_init`;
6. appeler `init(&world())`;
7. stocker `LoadedMod`.

Flow `load_mod_library` pour le bouton `.so` direct:

1. si le fichier choisi s'appelle `mod.json`, utiliser le flow manifest;
2. sinon charger la lib brute, creer un manifest synthetique avec filename comme id;
3. ce mode est dev-only et affiche "No manifest".

## 7. Settings UI

Modifier `src/dusk/ui/settings.cpp`.

Ajouter includes:

```cpp
#include "dusk/modding/mod_manager.hpp"
```

Ajouter un onglet `Mods`, probablement apres `Interface` ou avant `Cheats`, avec le pattern existant:

```cpp
add_tab("Mods", [this](Rml::Element* content) {
    auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
    auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);
});
```

Contenu v0:

- section `Management`;
- bouton `Load Mod...` qui appelle `ShowFileSelect`;
- bouton `Open Mods Folder` si desktop; ouvrir `data::configured_data_path() / "mods"`;
- liste des mods charges via `loaded_mods()`;
- a droite: details du mod selectionne, status, chemin, erreur recente.

Filtre `ShowFileSelect`:

- Linux: `so,json`
- Windows: `dll,json`
- macOS: `dylib,json`

Callback:

- convertir `path` en `std::filesystem::path`;
- si erreur SDL: modal "Mod Not Loaded";
- appeler `mod_manager`;
- jouer `kSoundItemChange` si succes, `kSoundWindowClose` si erreur;
- afficher erreur avec `Modal` comme `show_data_folder_error_modal()`.

Important: cette UI utilise Rml/Pane, pas ImGui.

## 8. Bridge Acteurs Et Lifecycle

`game_bridge.*` gere le miroir Flecs des acteurs natifs. Il ne doit pas dependre des appels individuels `fopAcM_create*`, car beaucoup de helpers passent par des chemins differents. Les hooks fiables sont dans le coeur acteur.

Points de branchement:

- creation: `src/f_op/f_op_actor.cpp`, dans `fopAc_Create`, quand `ret == cPhs_COMPLEATE_e`;
- destruction: `src/f_op/f_op_actor.cpp`, dans `fopAc_Delete`, quand `ret == TRUE`, avant que l'acteur sorte de `g_fopAcTg_Queue` et avant free memoire;
- fallback/reconciliation: `fopAcIt_Executor` dans `src/f_op/f_op_actor_iter.cpp`, appele une fois par frame pour detecter les acteurs rates.

API interne:

```cpp
namespace dusk::modding::game_bridge {
void initialize();
void shutdown();
void on_actor_created(fopAc_ac_c* actor);
void on_actor_destroyed(fopAc_ac_c* actor);
void sync_before_mods(float deltaSeconds);
}
```

Mapping:

- cle runtime: `fpc_ProcID` via `fopAcM_GetID(actor)`;
- stockage: `std::unordered_map<fpc_ProcID, flecs::entity>`;
- entity name optionnel: proc name si disponible, sinon `actor_<id>`;
- toujours ajouter `dusklight::Actor` + `dusklight::NativeActor`;
- tags selon `fopAcM_GetGroup(actor)`: `Player`, `Enemy`, `Npc`;
- tag `Item` pour les profils item connus (`fpcNm_ITEM_e`, `fpcNm_Demo_Item_e`, objets item direct-get), a affiner avec tests.

`dusklight::Actor` doit contenir les infos stables:

```cpp
struct Actor {
    uint32_t id;
    int16_t profile;
    int8_t room;
    uint8_t group;
    uint32_t params;
};
```

Creation:

1. `on_actor_created(actor)` ignore les acteurs deja connus.
2. Cree une entity Flecs et ajoute `Actor`, `NativeActor`, tags de groupe.
3. Copie `Transform` initial depuis `current.pos` / `current.angle`.
4. Envoie `world.event<events::ActorCreated>().entity(entity).emit({id})`.

Destruction:

1. `on_actor_destroyed(actor)` retrouve l'entity par id.
2. Envoie `ActorDestroyed` tant que `NativeActor.ptr` est encore valide.
3. Retire l'entree de la map.
4. `entity.destruct()` pour supprimer tous les composants et systems observers lies a cette entity.

Sync frame:

- parcourir les acteurs vivants avec `fopAcIt_Executor`;
- si un id n'existe pas dans la map, appeler `on_actor_created`;
- mettre a jour `Actor.room`, `Actor.params`, `Transform`, `NativeActor.ptr`;
- marquer les ids vus dans un set temporaire;
- pour tout id connu mais non vu, emettre `ActorDestroyed` et `destruct()`; ce fallback couvre les transitions de scene/room et les cas de delete non observes.

Ordre d'appel dans `m_Do_main.cpp`:

- apres `fapGm_Execute()` et `mDoAud_Execute()`: `game_bridge::sync_before_mods(dt)`;
- ensuite seulement `modding::progress(dt)`, pour que les mods voient un monde deja a jour.

Important: ne jamais laisser un `NativeActor.ptr` expose apres destruction. Apres `ActorDestroyed`, l'entity doit disparaitre le meme frame.

## 9. Bridge Player Minimal

Le player est une vue specialisee par-dessus le bridge acteurs.

Objectif v0:

- tagger l'entity de Link avec `Player`;
- garantir une query simple cote mod: `world.each<dusklight::Player, dusklight::Transform>(...)`;
- garder `NativeActor` pour debug/dev.

Sources utiles:

- player acteur: chercher via helpers existants autour de `daPy_getPlayerActorClass()` / `daPy_getLinkPlayerActorClass()`;
- infos actor: `fopAcM_GetID`, `fopAcM_GetName`, `fopAcM_GetProfName`, `fopAcM_GetRoomNo`, `fopAcM_GetPosition_p`, `fopAcM_GetAngle_p`.

Ne pas exposer d'ecriture moteur tant que les collisions, scene transitions et ownership ne sont pas definis.

## 10. Exemple Mod

Ajouter:

```txt
examples/mods/hello_flecs/
  CMakeLists.txt
  mod.json
  hello_flecs.cpp
```

L'exemple compile une shared lib, inclut `include/dusklight/sdk.hpp`, exporte:

```cpp
DUSK_MOD_INIT(world) {
    world->system<dusklight::events::Frame>()
        .kind(flecs::OnUpdate)
        .each([](const dusklight::events::Frame&) {});
}
```

Il doit etre buildable hors du jeu avec `DUSKLIGHT_SDK_DIR=/path/to/repo`.

## 11. Validation

Build:

```sh
cmake --preset linux-default-debug
cmake --build --preset linux-default-debug --target dusklight
cmake --build --preset linux-default-debug --target hello_flecs
```

Checks loader:

- charger un `mod.json` valide depuis Settings;
- charger un `.so` direct depuis Settings;
- verifier dans les logs que `dusk_mod_init` est appele;
- tester erreurs: fichier absent, JSON invalide, `sdk_version` incompatible, symbole absent.

Checks runtime:

- `flecs::world` cree une seule fois;
- composants publics visibles dans le mod;
- system Flecs execute chaque frame;
- creation d'un acteur via l'Actor Spawner cree une entity Flecs et emet `ActorCreated`;
- suppression/changement de room detruit l'entity et emet `ActorDestroyed`;
- les tags `Player`, `Enemy`, `Npc`, `Item` correspondent aux groupes/profils attendus;
- fermeture de Dusklight apres chargement sans crash;
- build ASan si preset disponible.

Checks UI:

- navigation clavier/controller dans `Mods`;
- nom long et message d'erreur long lisibles;
- fermeture/reouverture Settings garde la liste des mods;
- `Open Mods Folder` pointe vers `data::configured_data_path() / "mods"`.

## 12. Ordre D'Implementation

1. Ajouter headers publics SDK + versions.
2. Ajouter `sdk_world` et l'appeler depuis `m_Do_main.cpp`.
3. Ajouter `shared_library` + link `${CMAKE_DL_LIBS}` si necessaire.
4. Ajouter `mod_manifest`.
5. Ajouter `mod_manager`.
6. Ajouter onglet Settings `Mods` et callback `ShowFileSelect`.
7. Ajouter `hello_flecs`.
8. Ajouter `events::Frame` + `world.progress`.
9. Ajouter hooks `fopAc_Create/Delete` + map `Actor` -> entity Flecs.
10. Ajouter sync/reconciliation via `fopAcIt_Executor`.
11. Ajouter bridge Player `Transform` read-only.
12. Etendre tags/profils `Enemy`, `Npc`, `Item` avec tests en jeu.
