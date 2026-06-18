# Hello Flecs Mod

Build the example shared library:

```sh
cmake --build --preset linux-default-relwithdebinfo --target hello_flecs
```

Install it into the Dusklight data folder. The easiest way to find it is
Dusklight Settings > Interface > Open Data Folder, then create `mods/hello_flecs`
inside that folder:

```sh
mkdir -p /path/to/Dusklight-data/mods/hello_flecs
cp examples/mods/hello_flecs/mod.json /path/to/Dusklight-data/mods/hello_flecs/
cp build/linux-default-relwithdebinfo/examples/mods/hello_flecs/hello_flecs.so /path/to/Dusklight-data/mods/hello_flecs/
```

Launch Dusklight, open Settings > Mods, choose `Load Mod...`, and select
`mods/hello_flecs/mod.json`. The mod registers Flecs systems that read `dusk_frame`
(`dusklight::events::Frame`) and mirrored actor `dusklight::Transform` components.
It is intentionally silent; success is visible in the Settings modal and logs.
