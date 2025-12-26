## 0) Goals and guiding constraints

**You want:**
- A **Linux-first C++ app** that is **JACK-aware** (transport sync, low-latency audio callback).
- A **timeline arranger** where users drag “beat blocks” (e.g., *4/4 groove*, *fill*, *signature*) onto tracks.
- Blocks **snap/quantize** to grid and **stretch/shrink** by dragging edges, automatically adding/removing measures while preserving musical intent.
- Under the hood: patterns represented like **Hydrogen** (step/sequencer-style MIDI-ish note events).
- Playback prefers **sample output (drum sampler)** over external MIDI.
- Users can map instruments (kick/snare/etc.) to samples and adjust mix.

This architecture assumes: real-time audio thread is sacred; all heavy work is precomputed or done on non-RT threads.

---

## 1) Top-down architecture (major subsystems)

### A. Application shell
- Initializes logging, config, plugin/sample paths, JACK client.
- Creates **Engine** and **UI**; binds them through a stable API (command queue + state snapshots).
- Owns “project” persistence (save/load).

### B. Engine (real-time + scheduling)
Split into:
1. **Transport & Sync**
   - JACK transport integration (position, tempo, time signature, rolling state).
   - Internal transport if JACK transport isn’t running.
2. **Timeline/Arrangement Scheduler**
   - Converts high-level arrangement (“blocks on a timeline”) into **time-stamped note events** in **audio frames**.
3. **Drum Instrument & Sampler**
   - Sample-accurate triggering, envelopes, mixing, per-voice routing.
4. **Audio I/O**
   - JACK process callback renders audio buffers.
5. **Background Services**
   - Pre-render/caching of event lists per region, sample loading, waveform peaks for UI.

### C. Domain model (musical data)
- Project → Song → Timeline → Tracks → Regions → Patterns → Steps/Notes → Instruments/Samples.
- Time signature map & tempo map as first-class timeline entities.

### D. UI
- Timeline editor: drag/drop blocks, edge-trim/stretch, snapping grid, zoom, selection.
- Pattern editor (optional for MVP): step sequencer view per pattern.
- Mixer/instrument mapping view.
- Transport controls (play/stop/loop).
- Uses engine state snapshots and posts commands back.

---

## 2) Recommended libraries/frameworks (Linux/C++)

### Audio + Sync
- **JACK** (mandatory): `jack/jack.h`, `jack/transport.h`
- **libsamplerate** (optional): if you need sample-rate conversion for loaded samples.
- **libsndfile**: load WAV/AIFF/FLAC etc.

### UI
Two good choices:

**Option 1 (pragmatic, fast): Qt 6**
- Qt Widgets or QML.
- Great for drag/drop, custom timeline, docking panels.
- Audio thread stays in engine; UI uses signals/slots + command queue.

**Option 2 (audio-app oriented): JUCE**
- Strong audio utilities and UI, but JACK integration is possible yet you’ll still likely directly use JACK for transport specifics. JUCE is not as Linux-native in feel as Qt, but it works.

Given your JACK requirement + Linux preference, I’d pick **Qt 6 + direct JACK**.

### Serialization
- **nlohmann/json** for project files (fast to implement), or
- **YAML-CPP** if you prefer human-friendly files.
- Consider a custom “.drumproj” that’s a folder with JSON + samples.

### MIDI-like pattern representation
- Internally: your own minimal event structs (Hydrogen-like).
- Optional: **RtMidi** if later you add MIDI in/out (not required for sample output).

### Utility
- **fmt** for formatting
- **spdlog** for logging
- **Boost** only if needed (avoid if you can)

---

## 3) Core concept: Regions (“blocks”) on a musical timeline

### Region types
A region is an item on the timeline with:
- Start position (musical time)
- Length (musical time)
- A Pattern reference (or a fill/pattern variant)
- A “time behavior” policy when stretched

You’ll likely want at least:
1. **GrooveRegion**: repeats a base pattern over N measures (stretch changes measure count).
2. **FillRegion**: one-shot pattern that can be stretched but typically quantizes to bar length; stretching could choose a different fill variant or repeat last segment.
3. **SignatureRegion**: changes time signature at a position (not an audio region, but timeline event).
4. **TempoRegion**: optional, tempo automation.

### Stretch semantics (important)
Dragging edges should not “time-stretch audio”; it should change **musical length**:
- For step patterns: stretching changes the number of measures the pattern repeats (or truncates).
- If the user shrinks to partial measure, you can either:
  - disallow (snap region edges to bar boundaries), or
  - allow and truncate events beyond end.

For MVP: **snap region edges to bar boundaries**; later add finer snapping.

---

## 4) Time representation and snapping

### Internal time types
Use two time domains:
1. **Musical time** for UI editing: bar/beat/tick
2. **Audio time** for rendering: absolute sample frames

Proposed:
- PPQ resolution: e.g. **960 ticks per quarter note**.
- Musical position: `bar`, `beat`, `tick` (or just `int64_t ticks` from start with a meter map).

Define:
- `using Tick = int64_t;`
- `struct MusicalPos { Tick tick; };` (plus helpers to display as bar:beat:tick)

### Meter and tempo maps
- **TempoMap**: piecewise constant BPM initially.
- **MeterMap**: time signatures at tick positions.
Both maps are queried by:
- snapping logic (grid)
- conversion between tick ↔ seconds ↔ frames

### Snapping
Snapping is UI-driven but must align with engine rules:
- Snap to:
  - bars
  - beats
  - subdivisions (1/8, 1/16, triplets)
- Also “smart snap” to region boundaries.

---

## 5) Playback pipeline (from regions to audio)

### Step 1: Resolve arrangement to events
At playback time, for each track:
- Determine active regions in the current render window (e.g., current JACK cycle).
- For each region, generate note events for the time window:
  - Pattern → list of note hits in ticks
  - Apply region start tick offset
  - Apply repetition count based on region length
  - Filter to the window (lookahead is useful)

This can be:
- On-the-fly generation in a non-RT thread with caching, or
- Precompiled event lists per region when edited, then fast scanning in RT.

**Recommended**: maintain a **RegionEventCache** updated on edits (non-RT). RT thread only consumes a lock-free queue of events sorted by frame time.

### Step 2: Schedule sample triggers
Convert event tick → frame using tempo map + current transport state, then:
- Push `VoiceTrigger` events into a per-audio-block list.
- Trigger voices sample-accurately within the JACK process callback.

### Step 3: Sampler voice rendering
Sampler:
- Each trigger spawns a `VoiceInstance`:
  - sample pointer
  - playback position
  - gain/pan
  - optional envelope/decay
- Mix into output buffers (stereo).

For MVP: one-shot sample playback, no time-stretch, no pitch shift.

---

## 6) JACK integration details (expected behaviors)

Support:
- Create JACK client, register 2 out ports (and optionally multiple outs later).
- Implement `process(nframes)` callback:
  - read JACK transport state/position (if following).
  - render audio.
- Handle:
  - sample rate changes
  - buffer size changes
  - xrun notifications (log)

Transport:
- If JACK transport is rolling: follow.
- If not: allow internal transport but still output audio through JACK.
- For MVP: follow JACK transport only; provide start/stop via JACK transport commands optionally.

---

## 7) Suggested module layout (repository structure)

```
/src
  /app
    Main.cpp
    AppController.hpp/.cpp
  /engine
    Engine.hpp/.cpp
    JackAudioBackend.hpp/.cpp
    Transport.hpp/.cpp
    Scheduler.hpp/.cpp
    EventCache.hpp/.cpp
    Sampler.hpp/.cpp
    SampleLibrary.hpp/.cpp
    Mixer.hpp/.cpp
  /domain
    Project.hpp/.cpp
    Song.hpp/.cpp
    Timeline.hpp/.cpp
    TempoMap.hpp/.cpp
    MeterMap.hpp/.cpp
    Track.hpp/.cpp
    Region.hpp/.cpp
    Pattern.hpp/.cpp
    Instrument.hpp/.cpp
  /ui
    MainWindow...
    TimelineView...
    PatternEditor...
    MixerView...
  /serialization
    ProjectJson.hpp/.cpp
/tests
```

---

## 8) Domain model (key classes)

### Musical primitives
- `using Tick = int64_t;`
- `struct TimeSignature { int numerator; int denominator; };`
- `struct TempoBpm { double bpm; };`

### Pattern model (Hydrogen-like)
- `struct StepNote`
  - `int instrumentId`
  - `Tick offsetTick` (within pattern loop)
  - `float velocity` (0..1)
  - `float probability` (optional)
  - `int roundRobinGroup` (optional)
- `class Pattern`
  - `std::string id`
  - `std::string name`
  - `Tick lengthTicks` (typically 1 bar, but can be multi-bar)
  - `std::vector<StepNote> notes`

Hydrogen-like step grid can be derived from notes; you don’t have to store it as a grid unless the editor benefits from it.

### Regions on timeline
- `enum class RegionType { Groove, Fill, Signature, Tempo };`
- `class Region`
  - `std::string id`
  - `RegionType type`
  - `Tick startTick`
  - `Tick lengthTicks`
  - `std::string patternId` (for Groove/Fill)
  - `bool snapToBars` (MVP true)
  - `int trackIndex`
  - Stretch behavior:
    - `enum class StretchMode { Repeat, Truncate, VariantSelect };`
    - `StretchMode stretchMode`

### Track & Timeline
- `class Track`
  - `std::string name`
  - `std::vector<Region>` regions
  - `bool mute`, `bool solo`
- `class Timeline`
  - `TempoMap tempoMap`
  - `MeterMap meterMap`
  - `std::vector<Track> tracks`

### Instruments/samples
- `class Instrument`
  - `int id`
  - `std::string name`
  - `float gain`
  - `float pan`
  - `std::vector<std::string> samplePaths` (or a richer SampleSet)
  - `enum class TriggerMode { OneShot, ChokeGroup };` (optional)
- `class Sample`
  - `std::string path`
  - `int sampleRate`
  - `std::vector<float> interleaved` or planar L/R
- `class SampleLibrary`
  - load/cache samples, async load

---

## 9) Engine/UI boundary (stable API)

You want a clean boundary so Copilot can “stay on track.” Use:
- **Command queue** UI → Engine (non-RT)
- **State snapshot** Engine → UI (read-only copy)
- Optional: **event notifications** Engine → UI (playhead moved, xruns)

### Commands (UI → Engine)
Define a small set of command structs (variant/union). Examples:

- `CmdTransportPlay { }`
- `CmdTransportStop { }`
- `CmdSetPlayhead { Tick tick; }`
- `CmdSetLoop { bool enabled; Tick start; Tick end; }`

Editing:
- `CmdAddRegion { int trackIndex; Region region; }`
- `CmdMoveRegion { std::string regionId; Tick newStart; }`
- `CmdResizeRegion { std::string regionId; Tick newLength; }`
- `CmdDeleteRegion { std::string regionId; }`
- `CmdSetRegionPattern { std::string regionId; std::string patternId; }`

Tempo/meter:
- `CmdAddTimeSignature { Tick at; TimeSignature ts; }`
- `CmdAddTempo { Tick at; double bpm; }`

Instruments:
- `CmdAddInstrument { Instrument instrument; }`
- `CmdAssignInstrumentSample { int instrumentId; std::string samplePath; }`
- `CmdSetInstrumentMix { int instrumentId; float gain; float pan; }`

**Parameters to standardize across boundary**
- All time is **Tick** on the UI boundary.
- All IDs are stable strings (UUIDs) for regions/patterns.
- Track selection by index or stable id.

### State snapshot (Engine → UI)
- `struct EngineStateSnapshot`
  - `bool isPlaying`
  - `Tick playheadTick`
  - `double bpm`
  - `TimeSignature currentTS`
  - `Timeline timeline` (or a lightweight view model)
  - `std::vector<Instrument> instruments`
  - `int sampleRate`, `int bufferSize`
  - `uint64_t transportFrame` (optional)

For performance, you might avoid copying full timeline frequently:
- UI owns the editable document model.
- Engine owns a compiled/playback model.
- Snapshots then include only playhead + status; document changes are explicit via commands.

**Recommended approach**:
- UI owns the **authoritative Project document**.
- On edits, UI sends commands; engine updates its compiled model.
- Engine sends back only runtime state (playhead, roll state, warnings).

---

## 10) Quantization & self-quantize rules

When a user drops a region:
1. Compute drop tick from x-position.
2. Snap tick:
   - If region is “bar-based” (grooves): snap to nearest bar start in meter map.
   - If fill: snap to nearest beat or bar depending on fill type.
3. Set default length:
   - groove: 1 bar (or pattern length)
   - fill: pattern length

When resizing:
- If `snapToBars`: new length = N * barLengthTicks (based on meter at region start).
- Otherwise: snap to configured grid.

This keeps behavior musically predictable and avoids fractional measures early on.

---

## 11) Minimum Viable App (MVP) plan (continue)

### MVP scope (ship this first)
1. **JACK audio output (stereo)** + follow JACK transport (play/stop/position).
2. **Fixed tempo + fixed meter** initially (e.g., 120 BPM, 4/4) to reduce complexity.
3. **One timeline track** with:
   - drag/drop “Groove blocks” from a palette (predefined patterns)
   - **snap to bar lines**
   - **resize by dragging edges** (bar-quantized): resizing changes the number of bars (repeat/truncate)
4. **Sampler drum kit**
   - map instruments (kick/snare/hat) to WAV samples via a simple UI
   - one-shot playback, velocity controls
5. **Playback**
   - playhead, loop range (optional but helpful)
6. **Persistence**
   - save/load project (patterns + regions + instrument sample paths)

### MVP UI screens
- **Main window**
  - Transport bar: Play/Stop, tempo display (read-only), follow JACK toggle
  - Left palette: Pattern blocks (e.g., “Basic 4/4 Rock”, “8th Hats”, “Simple Fill”)
  - Center: Timeline (single track) with bar grid; drag blocks in; resize edges
  - Right: Instrument mapping (Kick → sample path, Snare → sample path, etc.) and gain

### MVP engine milestones (suggested order)
1. JACK client + process callback producing silence.
2. Sample loading via libsndfile and sampler that can trigger a single sample on command (manual test).
3. Transport following: convert JACK frame time → musical time (for fixed tempo) and schedule triggers.
4. Pattern playback (no timeline yet): loop a single 1-bar pattern repeatedly.
5. Timeline: schedule multiple regions with repeat/truncate behavior.
6. UI drag/drop + snapping + resize, sending commands to engine.
7. Save/load.

---

## 12) Post-MVP roadmap (incremental complexity)

### Phase 2: Multi-track + fills + choke groups
- Multiple tracks (or one drum track but layered regions).
- Fill regions that override groove for a span.
- Choke groups (open hat chokes when closed hat triggers).

### Phase 3: Meter map + tempo map (core to your original vision)
- Add **SignatureRegion** and **TempoRegion**.
- Snapping/grid adapts to meter sections.
- Region length quantization uses bar length at region start (or per-bar across region, depending on design choice).

### Phase 4: Pattern editor + Hydrogen-like UX
- Step sequencer editor:
  - instruments on rows, steps on columns
  - per-step velocity/probability
- Pattern library management (tags, favorites).

### Phase 5: Advanced scheduling + performance
- Precompiled event caches per region; incremental rebuild on edits.
- Lookahead scheduling for sample-accurate timing across JACK periods.
- Optional: offline render/export to WAV.

### Phase 6: MIDI I/O and plugin hosting (optional)
- MIDI out for external drum modules.
- LV2 hosting or LV2 drum plugins (bigger scope).

---

## 13) Key engineering decisions (to prevent rework)

### Decision A: Authoritative document ownership
To keep UI editing simple and deterministic:
- **UI owns the Project document** (editable, undoable).
- Engine maintains a **compiled playback model** derived from the document.
- UI issues editing commands; engine recompiles affected parts asynchronously, then swaps a pointer atomically.

Why: avoids the engine becoming a “document editor” and keeps RT thread isolated.

### Decision B: Compilation boundary
Define a compiled structure the RT thread consumes:

- **Document layer (editable)**: Patterns, Regions, Tempo/Meter maps.
- **Compiled layer (playback)**: sorted event lists per track/region, pre-resolved instrument mappings, precomputed bar lengths.

The compiled layer should be:
- immutable once published to audio thread
- swapped using atomic shared_ptr (or RCU-like approach)

### Decision C: Timing conversion strategy
For MVP (fixed tempo/meter):
- Tick ↔ frame conversion is constant.

For full version (tempo/meter map):
- Provide functions:
  - `Tick tickAtFrame(uint64_t frame, TransportState)`
  - `uint64_t frameAtTick(Tick tick, TransportState)`
Use piecewise integration over tempo segments (cache breakpoints).

---

## 14) Concrete class/interface plan (names + responsibilities)

### Engine façade (UI-facing)
**`class DrumEngine`**
- `bool startJack(const std::string& clientName);`
- `void stopJack();`
- `void postCommand(const EngineCommand& cmd);`  // lock-free or mutex + wake
- `EngineRuntimeState getRuntimeState() const;`  // atomic snapshot
- `void setProject(const ProjectDocument& doc);` // rebuild compiled model (non-RT)
- `void requestRecompile(RecompileHint hint);`

**Key parameters across boundary**
- all times in `Tick`
- stable IDs: `PatternId`, `RegionId` strings (UUID)

### Runtime state (engine → UI)
**`struct EngineRuntimeState`**
- `bool isJackRunning;`
- `bool isPlaying;`
- `bool followJackTransport;`
- `uint32_t sampleRate;`
- `uint32_t bufferSize;`
- `Tick playheadTick;`
- `uint64_t playheadFrame;`
- `double currentBpm;`
- `TimeSignature currentTS;`
- `uint32_t xruns;` (optional)
- `std::string lastError;` (optional)

### Commands (UI → engine)
Use `std::variant`:

**`using EngineCommand = std::variant<...>`**

Transport:
- `struct CmdPlay {};`
- `struct CmdStop {};`
- `struct CmdSetFollowJack { bool enabled; };`
- `struct CmdSetPlayhead { Tick tick; };`
- `struct CmdSetLoop { bool enabled; Tick start; Tick end; };`

Document edit notifications (engine rebuild triggers):
- `struct CmdDocumentChanged { uint64_t revision; RecompileHint hint; };`
  - UI increments a revision number whenever it changes the doc.
  - Engine requests latest doc through `setProject()` or a shared doc pointer (choose one; simplest is `setProject(copy)` for MVP, more efficient is shared immutable snapshots later).

Instrument/sample:
- `struct CmdPreviewHit { int instrumentId; float velocity; };` (nice UX)
- `struct CmdSetInstrumentGain { int instrumentId; float gain; };`
- `struct CmdSetInstrumentPan { int instrumentId; float pan; };`

### Document model (UI-owned)
**`class ProjectDocument`**
- `std::string name;`
- `double baseBpm;` (or tempo map later)
- `TimelineDocument timeline;`
- `PatternLibrary patterns;`
- `InstrumentRack instruments;`
- `uint64_t revision;`

**`class TimelineDocument`**
- `std::vector<TrackDocument> tracks;`
- `TempoMapDocument tempoMap;` (MVP: constant tempo)
- `MeterMapDocument meterMap;` (MVP: constant 4/4)

**`struct TrackDocument`**
- `std::string id; std::string name;`
- `std::vector<RegionDocument> regions;`

**`struct RegionDocument`**
- `std::string id;`
- `RegionType type;` (MVP: Groove only)
- `Tick startTick;`
- `Tick lengthTicks;`
- `std::string patternId;`
- `StretchMode stretchMode;` (MVP: Repeat)
- `bool snapToBars;` (MVP: true)

**`struct PatternDocument`**
- `std::string id; std::string name;`
- `Tick lengthTicks;` (usually 1 bar)
- `std::vector<StepNote> notes;`

**`struct StepNote`**
- `int instrumentId;`
- `Tick offsetTick;`
- `float velocity;`

**`struct InstrumentDocument`**
- `int id; std::string name;`
- `float gain; float pan;`
- `std::vector<std::string> samplePaths;` (MVP: single sample path is enough)

### Compiled model (engine-owned)
**`struct CompiledProject`**
- `uint64_t sourceRevision;`
- `CompiledTimeline timeline;`
- `std::vector<CompiledInstrument> instruments;`

**`struct CompiledRegion`**
- `RegionId id;`
- `uint64_t startFrame;` and/or `Tick startTick;`
- `Tick lengthTicks;`
- `std::vector<CompiledEvent> events;` (pre-expanded for repeats) OR a pointer to pattern + repetition metadata

**`struct CompiledEvent`**
- `uint64_t frame;` (absolute or relative-to-region)
- `int instrumentId;`
- `float velocity;`

For MVP, you can compile events into **relative frames within one bar** and expand repeats at runtime with a simple loop; later pre-expand for long regions if needed.

### Scheduler & audio thread
**`class Scheduler`**
- Input: `CompiledProject` + transport position each cycle
- Output: list of triggers that fall into `[cycleStartFrame, cycleEndFrame)`
- Should be RT-safe: no allocations, no locks in process callback

**`class JackAudioBackend`**
- owns JACK client, ports
- `static int process(jack_nframes_t nframes, void* arg);`
- calls `engine->render(nframes, outL, outR)`

**`class SamplerEngine`**
- `void noteOn(int instrumentId, float velocity, uint32_t offsetFramesWithinBlock);`
- `void render(float* outL, float* outR, uint32_t nframes);`
- manages active voices pool (fixed-size vector for RT safety)

---

## 15) Timeline editing behaviors (precise rules)

### Drag/drop a pattern block
Inputs:
- drop x-position → `Tick dropTick`
- selected pattern has `lengthTicks`

Rules (MVP):
- `startTick = snapToBar(dropTick)`
- `lengthTicks = pattern.lengthTicks` (usually 1 bar)
- create region with `StretchMode::Repeat`, `snapToBars=true`

### Resize region edge
User drags right edge; compute raw length:
- `rawLength = snapToBar(endTick) - region.startTick`
- enforce `rawLength >= 1 bar` (optional)
- set region.lengthTicks = rawLength

Playback meaning:
- region plays pattern repeated to fill region length
- any partial remainder truncated (but in MVP you’ll only allow bar multiples)

### Move region
- `startTick = snapToBar(rawStartTick)`
- preserve length

### Overlaps
For MVP, simplest policy:
- allow overlaps and sum them (layering)
or
- prevent overlaps on the same track (reject drop)
Pick one early. For drum grooves/fills, you’ll later want precedence rules; for MVP, **prevent overlaps** to avoid ambiguity.

---

## 16) How “signature blocks” fit later (your original requirement)

When you introduce time signatures, treat them as **timeline events** (like markers), not audio regions on a track.

### Data model
- Replace/augment `SignatureRegion` with a meter-map event:

**`struct MeterChange { Tick atTick; TimeSignature ts; };`**  
Stored in `MeterMapDocument` (sorted by `atTick`).

### UI behavior (“drag 4/4 onto timeline”)
- The palette item “4/4” creates a `MeterChange` at the dropped tick.
- It snaps to a **bar boundary** (or nearest beat boundary; bar boundary is saner).
- Stretching a “signature block” is conceptually defining a range where that signature applies. Implementation-wise:
  - You can **display it as a block** for UX,
  - but persist it as: start meter change + an implicit end (next change).
- When the user stretches the right edge, you’re really moving the **next** meter change (or inserting one).

### Editing rules (recommended)
To avoid complex meter math early:
- **Rule A (strongly recommended): meter changes occur only at bar starts** (snap hard).
- **Rule B: regions may not cross meter changes** in early versions:
  - if user drags a region across a meter change, either:
    - auto-split into two regions, or
    - block the move/resize with a visual constraint.
This keeps “repeat N measures” deterministic.

Later, if you want regions to cross meter changes, you’ll need “measure counting” across varying bar lengths.

---

## 17) Tempo map (“drag tempo” / automation) integration

Tempo changes behave similarly to meter changes.

### Data model
**`struct TempoChange { Tick atTick; double bpm; };`**

### Playback conversion
When tempo is variable, tick↔frame is no longer linear. You’ll need:
- piecewise conversion across tempo segments
- caching to keep it fast

Recommended internal representation:
- store tempo changes sorted by tick
- build a cache of “anchors”:
  - for each tempo segment start tick `Ti`, store corresponding absolute seconds (or frames) `Fi`
  - then within segment: `frames = Fi + (tick - Ti) * framesPerTick(tempoAtSegment)`

That makes `frameAtTick()` O(log N) with binary search + O(1) math.

For `tickAtFrame()`, you do binary search on anchors by frame.

---

## 18) Scheduling strategy (lookahead + determinism)

JACK calls you with blocks (periods). If you schedule exactly “events in this period,” you risk missing events if:
- tempo/meter conversion yields events near the edge
- UI edits occur mid-play

Recommended approach:
- Maintain a **lookahead window** in frames (e.g., 2–4 JACK periods).
- A non-RT scheduler thread computes upcoming triggers into a lock-free ring buffer.
- The RT thread only consumes triggers whose frame falls inside the current callback window.

### Two-tier design
1. **Compiler (non-RT)**: turns document → compiled model (events, patterns, etc.).
2. **Scheduler (non-RT)**: reads compiled model + transport, writes trigger queue.
3. **Audio RT**: reads trigger queue and renders sampler.

MVP can collapse scheduler into RT if tempo/meter are fixed and everything is trivial, but given your end goals, building the queue approach early prevents a rewrite.

---

## 19) Hydrogen-like pattern features (mapping to your model)

Hydrogen patterns include:
- per-instrument patterns (rows)
- steps with velocity
- swing/humanize
- probability
- flam/roll
- multiple layers

You can accommodate this gradually without breaking your core architecture by extending `StepNote`:

**`struct StepNote`**
- `int instrumentId`
- `Tick offsetTick`
- `float velocity`
- `float probability` (default 1.0)
- `int variationGroup` (optional)
- `HumanizeParams humanize` (optional)
- `Articulation articulation` (normal/flam/roll)

And a track-level/global timing modifier:
- `SwingParams { float amount; int division; }` (apply in compiler when generating event ticks)

Crucially: keep the **engine compiled event list** the single source of truth for playback timing. Humanize/swing should be applied at compile-time (or schedule-time), not in the RT audio thread.

---

## 20) Instrument voice assignment (sample output instead of MIDI)

### Instrument mapping UX
Like Hydrogen:
- Instruments list: Kick, Snare, Hat, etc.
- Each instrument has:
  - one or more samples (for round-robin/velocity layers later)
  - gain, pan
  - choke group (for hats)

### Sampler design (MVP → scalable)
MVP:
- one sample per instrument
- fixed polyphony limit (e.g., 64 voices global)
- simple linear interpolation or none (if sample rates match JACK)

Later:
- velocity layers: choose sample based on velocity
- round robin: choose next sample
- choke: stop voices in same choke group
- per-instrument outputs: multiple JACK ports (optional)

RT safety:
- all samples preloaded/decoded before playback
- sampler uses a preallocated voice pool (no `new` in RT)

---

## 21) Persistence (project file format)

### MVP-friendly format
A single JSON with relative paths is simplest:

`project.json`:
- tempo (or tempo map)
- meter (or meter map)
- patterns
- regions (track + start + length + patternId)
- instruments (id/name/gain/pan/samplePaths)

Optionally:
- create a project folder:
  - `project.json`
  - `samples/` (copied or referenced)
This makes projects portable.

### Versioning
Include:
- `fileFormatVersion: int`
- migrate on load

---

## 22) Undo/redo (strongly recommended for timeline editing)

Qt gives you **QUndoStack** which is perfect:
- Each UI edit operation becomes a command:
  - AddRegionCommand
  - MoveRegionCommand
  - ResizeRegionCommand
  - AddMeterChangeCommand
- On commit, also notify engine with `CmdDocumentChanged {revision, hint}`.

This keeps editing coherent and reversible.

---

## 23) Real-time safety checklist (non-negotiables)

In JACK `process()`:
- no locks (mutex), no file I/O, no memory allocation
- no logging (or only lock-free ring logger)
- only read atomically published pointers and ring buffers

Preload:
- samples
- compiled events
- anything used in audio callback

Communication:
- UI → engine: queue (mutex ok) handled outside RT
- scheduler → RT: lock-free ring buffer of triggers

---

## 24) Testing strategy (so implementation stays on track)

### Unit tests (no JACK)
- Tick/grid snapping logic with meter map.
- Pattern repeat/truncate event generation:
  - given pattern length + region length, verify events emitted at correct ticks.
- Tempo map conversions:
  - tick→frame and frame→tick round-trip within tolerance.

### Integration tests (with JACK optional)
- Headless engine with fake audio backend:
  - render N blocks and ensure triggers produce non-zero samples.
- Save/load project and compare document equality.

---

## 25) Summary “North Star” implementation sequence (recommended)

1. **Domain model** (ProjectDocument, PatternDocument, RegionDocument, InstrumentDocument).
2. **Fixed tempo/meter time math** (Tick↔frame).
3. **SamplerEngine** (voice pool, trigger, render).
4. **CompiledProject** builder (document → compiled events for a bar/region).
5. **JACK backend** with engine render.
6. **Minimal UI timeline** (Qt): draw grid, draw regions, drag/drop, resize; send doc-changed.
7. **Persistence** (JSON).
8. Add: multi-track, fills, meter map, tempo map, lookahead scheduler thread.

---

## 26) One-page “engine/UI boundary contract” (copy into your IDE notes)

**Time unit:** `Tick` (int64), PPQ=960.  
**IDs:** stable strings for patterns/regions; instruments by int id.  
**UI owns document; engine owns compiled playback model.**  
**UI notifies engine by sending CmdDocumentChanged(revision,hint) and then provides the updated ProjectDocument via setProject().**  
**Audio thread consumes only immutable compiled data + lock-free trigger queue; no allocations/locks.**

---

## Example event-flow: drop groove, stretch, add fill, change to 3/4

Assumptions for clarity:
- **PPQ = 960 ticks/quarter**
- **4/4 bar length** = 4 quarters = `4 * 960 = 3840 ticks`
- **3/4 bar length** = 3 quarters = `2880 ticks`
- **Tempo fixed** at 120 BPM for this example (tempo map exists but constant)
- **Rule B (early version)** enforced: **regions may not cross meter changes** (engine will auto-split or UI prevents). I’ll show **auto-split** at meter change.

We’ll use:
- Pattern `P_GROOVE_4_4` length = 1 bar (3840 ticks)
- Pattern `P_FILL_4_4` length = 1 bar (3840 ticks)
- Instruments: Kick=1, Snare=2, Hat=3

### Patterns (simplified)
**Groove pattern `P_GROOVE_4_4` (1 bar)**
- Hat eighths: ticks `[0, 480, 960, 1440, 1920, 2400, 2880, 3360]` vel 0.6
- Kick: ticks `[0, 1920]` vel 0.9
- Snare: ticks `[960, 2880]` vel 0.85

**Fill pattern `P_FILL_4_4` (1 bar)**
- Snare 16ths last half-bar, etc. (details not crucial; it’s just a different event list in that bar)

---

# Step 1: User drops “4/4 time” onto the track (timeline)

### UI action
User drags “4/4 Time” block to the very beginning.

### Snap
Drop position snaps to bar start tick 0.

### Document change
`MeterMapDocument` becomes:

- `MeterChange(atTick=0, ts=4/4)`

(If the project already defaults to 4/4 at tick 0, this is either a no-op or it explicitly inserts/overwrites.)

### Engine compile impact
- Recompute meter grid cache (bar start ticks).
- No audio regions yet, so no scheduled drum events.

---

# Step 2: User drops a “4/4 groove” block at bar 1 and stretches to 8 measures

Interpretation: “bar 1” here means the first bar of the song (starting at tick 0). Many UIs label that as measure 1. I’ll use **bar index** starting at 0.

### 2a) Drop groove at start

**UI drop → startTick**
- dropped near start → snapped to tick `0`.

**Initial region**
- startTick = 0
- lengthTicks = 3840 (1 bar)
- patternId = `P_GROOVE_4_4`
- stretchMode = Repeat
- snapToBars = true
- regionId = e.g. `R_GROOVE_A`

Document inserts:

`RegionDocument { id=R_GROOVE_A, type=Groove, startTick=0, lengthTicks=3840, patternId=P_GROOVE_4_4 }`

### 2b) Stretch to 8 measures

User drags right edge out to measure 9 start (i.e., 8 full bars total). With snap-to-bars, the UI computes:

- newEndTick = barStart(8) = `8 * 3840 = 30720`
- newLengthTicks = `30720 - 0 = 30720`

Document updates region:
- `R_GROOVE_A.lengthTicks = 30720`

### What “Repeat” means in compilation
The region represents **8 repetitions** of the 1-bar pattern.

- repetitions = `lengthTicks / pattern.lengthTicks = 30720 / 3840 = 8`

---

# Step 3: User drops a “fill” at bar 7 (i.e., replacing groove for one bar)

We need to define precedence: typically a fill *overrides* groove for its time range.

Two common ways:
1. Put fill on the **same track**, and enforce “topmost wins” by lane priority.
2. Have a dedicated “Fill track” with higher priority.
3. Keep one track but define region types with precedence rules.

I’ll assume: **same track**, and precedence is:
- FillRegion overrides GrooveRegion when overlapping.
- Engine compilation resolves overlaps by splitting/omitting underlying groove events during fill span.

### UI action
User drags `P_FILL_4_4` to bar 7 start.

Bar 7 start tick (0-based) = `7 * 3840 = 26880`.

Fill region defaults:
- startTick = 26880
- lengthTicks = 3840 (1 bar)
- id = `R_FILL_1`
- patternId = `P_FILL_4_4`
- type=Fill
- snapToBars=true

Document inserts:

`RegionDocument { id=R_FILL_1, type=Fill, startTick=26880, lengthTicks=3840, patternId=P_FILL_4_4 }`

### Overlap resolution in compilation
Groove spans [0, 30720). Fill spans [26880, 30720).

So the last bar of the groove (bar index 7) overlaps the fill exactly.

**Compilation result (conceptual):**
- Groove active bars: 0–6 (7 bars)
- Fill active bar: 7 (1 bar)

You can implement this by:
- building a “coverage” map per bar (or per tick interval) with precedence rules
- generating events from whichever region wins for that interval

---

# Step 4: User changes signature to 3/4 at bar 9 (start of bar index 8)

If the groove was stretched to 8 bars, it ends at tick 30720, which is the start of bar index 8 (i.e., measure 9 if 1-based labeling). That’s a nice boundary.

### UI action
User drags “3/4 Time” block to bar 9 start (tick 30720). Snaps to bar start tick 30720.

Document adds meter change:
- `MeterChange(atTick=30720, ts=3/4)`

Now meter map:
- at 0: 4/4
- at 30720: 3/4

### Consequences for grid
- Bars 0–7 are 4/4 (3840 ticks each)
- Starting at tick 30720, bars are 3/4 (2880 ticks each)

### Region crossing meter change rule
Our groove region ends exactly at 30720, so it does *not* cross. Fill ends at 30720. Great—no split needed.

If any region crossed 30720, engine would auto-split at 30720 (or UI would prevent).

---

# Final document snapshot (high level)

## Meter map
- `M0: atTick=0 -> 4/4`
- `M1: atTick=30720 -> 3/4`

## Regions on Track 0
1. `R_GROOVE_A` Groove
   - startTick=0
   - lengthTicks=30720 (8 bars of 4/4)
   - pattern=P_GROOVE_4_4
2. `R_FILL_1` Fill
   - startTick=26880 (bar 7)
   - lengthTicks=3840 (1 bar)
   - pattern=P_FILL_4_4

---

# What compiled playback looks like (tick-level first)

We’ll show which pattern is active per bar in the 4/4 section:

- Bar 0 (tick 0..3839): Groove
- Bar 1 (3840..7679): Groove
- Bar 2 (7680..11519): Groove
- Bar 3 (11520..15359): Groove
- Bar 4 (15360..19199): Groove
- Bar 5 (19200..23039): Groove
- Bar 6 (23040..26879): Groove
- Bar 7 (26880..30719): **Fill** (overrides groove)
- At tick 30720: meter switches to 3/4; no drum regions defined yet beyond this point.

---

# Compiled events (example subset)

Let’s list events for the first two bars to show how repetition works. I’ll use shorthand:
- `H` = Hat (inst 3) vel .6
- `K` = Kick (inst 1) vel .9
- `S` = Snare (inst 2) vel .85

### Bar 0 base groove events (relative ticks in bar)
Hat: 0, 480, 960, 1440, 1920, 2400, 2880, 3360  
Kick: 0, 1920  
Snare: 960, 2880

### Absolute tick events for Bar 0 (offset 0)
- tick 0: H, K
- tick 480: H
- tick 960: H, S
- tick 1440: H
- tick 1920: H, K
- tick 2400: H
- tick 2880: H, S
- tick 3360: H

### Absolute tick events for Bar 1 (offset 3840)
- tick 3840: H, K
- tick 4320: H
- tick 4800: H, S
- tick 5280: H
- tick 5760: H, K
- tick 6240: H
- tick 6720: H, S
- tick 7200: H

…and so on through bar 6.

### Bar 7 (offset 26880): fill events
Instead of groove events, compiler emits `P_FILL_4_4` events offset by 26880.
- e.g., tick 26880: K
- tick 27360: S
- … (fill-specific)

---

# Conversion to audio frames (because scheduler ultimately needs frames)

At 120 BPM:
- 1 quarter note = 0.5 seconds
- 4/4 bar = 2.0 seconds
- With sample rate SR (say 48k), one bar = 96,000 frames

Tick duration:
- quarter = 960 ticks = 0.5 s → 1 tick = 0.5/960 s
- framesPerTick = SR * 0.5 / 960
  - at SR=48000: framesPerTick = 48000 * 0.5 / 960 = 25 frames/tick

So (at 48k/120 BPM):
- tick 0 → frame 0
- tick 480 → frame 12,000 (an eighth note)
- tick 3840 (one bar) → frame 96,000
- tick 26880 (bar 7 start) → frame 26880 * 25 = 672,000
- tick 30720 (meter change) → frame 768,000

In reality, if you follow JACK transport, your “frame 0” is whatever frame corresponds to song start; but the math is the same relative to transport origin.

---

# What happens at runtime (queues and compilation)

Below is the same scenario, but described as **runtime message flow** between UI → Engine compiler → Scheduler → JACK RT audio.

---

## A) UI edits (document + undo + notify engine)

The UI owns `ProjectDocument` and maintains `revision` (monotonic).

### 1) Drop 4/4 at start
- UI executes `UndoCommand_AddMeterChange(atTick=0, ts=4/4)`
- Document revision becomes `rev=1`
- UI notifies engine:

**UI → Engine**
- `engine.setProject(docCopyOrSnapshot(rev=1))`
- `engine.postCommand(CmdDocumentChanged{revision=1, hint=RecompileHint::MeterMapChanged})`

Engine may ignore the command if `setProject()` already implies compile; or `CmdDocumentChanged` can be the trigger that causes the engine to request the latest doc snapshot—either is fine as long as it’s consistent.

### 2) Drop groove region at tick 0
- UI inserts `R_GROOVE_A` (1 bar)
- `rev=2`
- notify engine: `RecompileHint::RegionsChanged`

### 3) Resize groove to 8 bars (length=30720)
- UI updates `R_GROOVE_A.lengthTicks`
- `rev=3`
- notify engine: `RecompileHint::RegionsChanged`

### 4) Drop fill at bar 7 (tick 26880)
- UI inserts `R_FILL_1`
- `rev=4`
- notify engine: `RecompileHint::RegionsChanged`

### 5) Drop 3/4 meter at tick 30720
- UI inserts meter change
- `rev=5`
- notify engine: `RecompileHint::MeterMapChanged`

---

## B) Engine compilation (non-RT thread)

The engine has a **Compiler thread** (or uses a worker pool) that reacts to document updates.

### Inputs
- `ProjectDocument` snapshot @ revision N
- pattern library, instrument rack, regions, meter/tempo maps

### Outputs
- `std::shared_ptr<const CompiledProject> compiled`
- Atomically published for scheduler/RT to consume

### What compiler does for this example

#### 1) Validate + normalize
- Ensure meter changes are sorted, no duplicates at same tick (or last-wins).
- Ensure region edges align to bar boundaries if `snapToBars==true` (optional sanity pass).
- Apply “no-cross-meter-change” rule:
  - if a region spans a meter change tick, split:
    - `R1: start..meterChangeTick`
    - `R2: meterChangeTick..end` (may require pattern adaptation)
  In our case, no split needed.

#### 2) Resolve overlap precedence (fill overrides groove)
For Track 0, build an interval set (tick ranges) that picks the “winning” region per time slice:
- Groove interval: `[0, 30720)`
- Fill interval: `[26880, 30720)` overrides

Resulting effective intervals:
- `[0, 26880) -> Groove`
- `[26880, 30720) -> Fill`

(If you later allow finer-than-bar fills, this becomes more granular than bars.)

#### 3) Compile events
For each effective interval:
- Determine the pattern, pattern length in ticks, and repetition/truncation.
- Generate a sorted list of `CompiledEvent { tick, instrumentId, velocity }` OR directly `frame` if tempo map known.

For the example:
- Interval A `[0,26880)` = 7 bars of groove:
  - events = groove pattern repeated 7 times, offset by bar start
- Interval B `[26880,30720)` = 1 bar of fill:
  - events = fill pattern once, offset 26880

#### 4) Produce a compiled representation that RT can read fast
Two common forms:

**Form 1 (tick events + fast tick→frame conversion in scheduler)**
- store `CompiledEvent.tick` and let scheduler convert to frames based on tempo map and current transport origin.

**Form 2 (absolute frame events)**
- precompute event frames at compile time assuming a fixed mapping of tick-to-frame anchored at song start.
- If you’re strictly following JACK transport with a known mapping (tick 0 == transport frame 0), this can work well.

Given variable tempo in your future, the cleaner long-term approach is:
- store events in ticks (musical domain)
- scheduler converts ticks to frames using tempo map + transport position each time it schedules (with caching)

For MVP fixed tempo, either is fine.

---

## C) Scheduler (non-RT, continuously fills trigger queue)

Scheduler runs when:
- transport is rolling
- compiled project is available
- and/or document revision changes

### Inputs
- Current JACK transport state (frame, rolling)
- `CompiledProject` pointer (immutable)
- Loop state (if any)

### Output
A lock-free ring buffer of triggers:

`struct Trigger { uint64_t frame; int instrumentId; float velocity; };`

### Lookahead behavior
At time `nowFrame`, scheduler ensures triggers exist up to:
- `nowFrame + lookaheadFrames`
where `lookaheadFrames = periodSize * 4` (example)

Algorithm sketch (conceptual):
1. Compute current playhead tick from transport frame (or read tick if you keep it).
2. Determine scheduling window in frames: `[scheduleCursorFrame, targetFrame)`
3. Convert that window to tick range: `[tickA, tickB]` using tempo map
4. Scan compiled tick events in that tick range and push triggers with computed absolute frames.

For this example at SR=48k and 120 BPM:
- A hat at tick 480 becomes trigger at frame 12,000 relative to song start (plus any transport origin offset).

---

## D) JACK RT audio callback (hard real-time)

In each `process(nframes)`:
1. Determine the absolute frame range for this callback:
   - `[cycleStartFrame, cycleEndFrame)`
2. Pop all triggers from ring buffer whose `frame` lies in that range.
3. For each trigger:
   - call `sampler.noteOn(instrumentId, velocity, offsetFramesWithinBlock)`
     - where `offsetFramesWithinBlock = trigger.frame - cycleStartFrame`
4. Render the sampler voices into output buffers for `nframes`.

No document access, no compilation, no tempo math heavy lifting inside RT—only consuming triggers.

---

# Concrete scheduling example (numbers)

Assume:
- SR=48,000
- tempo=120 BPM
- framesPerTick=25 (as derived)
- JACK period = 256 frames
- playback starts at song tick 0 aligned to transport frame 0

### A few triggers in Bar 0
- tick 0 (K+H) → frame 0
- tick 480 (H) → frame 12,000
- tick 960 (S+H) → frame 24,000
- tick 1920 (K+H) → frame 48,000
- tick 3360 (H) → frame 84,000

Scheduler will push those triggers early, long before RT reaches those frames.

### Transition at bar 7 to fill
Bar 7 start:
- tick 26880 → frame 672,000

All groove events that would have occurred in bar 7 are *not* present in compiled event list for `[26880,30720)`. Instead fill events are present.

So the scheduler naturally produces fill triggers in that interval—no special runtime override logic needed in RT.

### Meter change at tick 30720
- tick 30720 → frame 768,000

Meter change primarily affects:
- snapping/grid and barline display
- how future “bar N” computations work
- if you have regions beyond that point, their bar quantization uses 3/4 bar length (2880 ticks)

If playback continues past 768,000 frames with no regions, scheduler emits no triggers (silence).

---

# Optional: how engine would auto-split if a region crossed the meter change

Suppose the user had stretched groove to **10 bars of 4/4** (end tick 38400), then added 3/4 at 30720.

With the “no-cross-meter-change” rule, compiler would split `R_GROOVE_A`:

- `R_GROOVE_A1`: `[0,30720)` in 4/4 → 8 bars (ok)
- `R_GROOVE_A2`: `[30720,38400)` now lies in 3/4 territory

But `R_GROOVE_A2` length is `7680 ticks`, which equals:
- `7680 / 2880 = 2.666...` bars of 3/4 (not bar-aligned)

What to do?
- If `snapToBars` is true, you’d typically **quantize the split remainder**:
  - either truncate to 2 full 3/4 bars (5760 ticks),
  - or extend to 3 full 3/4 bars (8640 ticks),
  - or disallow crossing in UI to avoid this ambiguity.

This is why an early constraint “regions can’t cross meter changes” (enforced at UI time) saves a lot of edge-case policy.

