# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
cmake -S . -B build
cmake --build build
AudioPluginHost.app/Contents/MacOS/AudioPluginHost Plugin.filtergraph
```

## Architecture

Semaforte is a JUCE audio plugin (AU/VST3/Standalone) that processes MIDI input to control audio crossfading between buses.

**Core Components:**
- `PluginProcessor` - Main plugin processor handling MIDI and audio
- `PluginEditor` - GUI with 2x6 grid layout
- `CrossFader` - Smoothed audio crossfader between buses using `juce::SmoothedValue`
- `MidiDebouncer` - Filters rapid MIDI messages, allowing only one per configurable time window
- `LongPressButton` - SVG-based button with short click and long press detection modes

**File Organization:**
- Headers in `include/`
- Implementations in `source/`
- JUCE framework in `libs/juce/`

## Code Style

- No space between method/function names and parentheses: `foo(x)` not `foo (x)`
- Member variables use underscore suffix: `audioProcessor_` not `audioProcessor`
- Use JUCE naming conventions (camelCase for methods, PascalCase for classes)
- Split interface (header) and implementation (cpp)
- Each class should have its own files

## Commit Messages

- Start with capital letter
- No trailing period
- Use imperative mood: `Fix bug` not `Fixed bug` or `Fixes bug`
- Keep concise (ideally under 50 characters)
