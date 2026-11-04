# Assets
In Euphrosyne, where you place your assets and what you name them is important, as it is how
the cooker works without requiring verbose metadata.
# Shaders 
- Placed in the `assets/shaders` directory.
- They use HLSL syntax.
- Compiled to SPIR-V by the cooker
- All are compiled and loaded while the engine is initializing
- Cooks to the the archive file `shaders.bin` placed next to the executable.
- Their type is determined by their file extension:

| Extension | Type |
|-----------|------|
| `.vs.hlsl` | Vertex Shader |
| `.ps.hlsl` | Pixel Shader |
| `.cs.hlsl` | Compute Shader |
|`.gs.hlsl` | Geometry Shader |
|`.hs.hlsl` | Hull Shader |
|`.ds.hlsl` | Domain Shader |


# SFX
- Not streamed or compressed, loaded into memory in their entirety. 
- Quicker playback at the cost of memory and disk usage
- Use for sounds shorter than 5 seconds
- Placed in the `assets/sfx` directory
- Supported formats for the input files are `.wav`,  `.flac`, `.mp3` and `.ogg`. 
- Cooks to the archive file `sfx.bin` placed next to the executable.
- You could technically use the SFX system for regular audio, but you'll flood memory honestly.

# Audio
- Streamed and compressed
- Slower playback but lower disk and memory use
- Use for sounds longer than 5 seconds
- Placed in the `assets/audio` directory
- Supported formats for the input files are `.wav`,  `.flac`, `.mp3` and `.ogg`.
- Cooks to the archive file `audio.bin` placed next to the executable.
- You could use the audio system for SFX if you care a lot about disk and memory usage