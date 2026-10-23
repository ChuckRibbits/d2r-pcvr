#pragma once
// VR Settings content: the words of the redesigned menu (labels, help, option
// names and hints, units, warn lines, pages, sections, presets, guide), minus
// LootPick and MenuPointer (not on upstream main).
//
// Data only: no drawing, no layout, no input. The rules:
// - Never restate a range or default. Min, max, step and default always come
//   from Definitions[key] (ini units); a row only says how to show them
//   (iniScale, unit, format). Saved ini files load unchanged.
// - Every Key appears in exactly one row, except NotInMenu.
// - At most 8 rows a page can show (title screen: every class row), 3 sections.
// - Labels <= 26 characters (Skills go to the aimed monster: main's own 30),
//   help <= 160, warn <= 56, option labels <= 14, sentence case, American
//   spelling, one vocabulary across pages.
// - Every help sentence is true of the code (vr_settings_help.h holds the long
//   form); where a draft said more than the code does, the code wins.
// - Presets leave out StereoRendering, DeferGpuWait, ElementalHands,
//   CompanionAi and the calibration keys (the owner's calls).
// The checks at the end of this file enforce these at compile time.
#include "vr_settings.h"
#include "vr_settings_draw.h"
#include <cmath>
#include <cstdio>
#include <iterator>
#include <limits>
#include <string>
namespace framevr::settings::content {
using draw::Icon;

// Rail groups: a thin divider between groups. Tools: Presets heads the rail;
// Header: Guide and What's new, opened from the page band.
enum class Group : uint8_t { Tools, You, Fight, Info, World, Device, Header };

// How a row is controlled.
enum class RowKind : uint8_t {
    Toggle,    // Off / On pill (Definitions max==1)
    Choice,    // 2-5 options: chips, every option visible
    Cycler,    // 6+ options, or a wrapping / class-dependent list: < value > with "n of m"
    Stepper,   // - value + with no bar (no upper limit, e.g. Melee hit speed)
    Slider,    // - value + with a fill bar and the default tick; drag to scrub
    Swatch,    // hue: < swatch name > cycler, or the card's color grid
    Button,    // an action, not a value (Calibrate body)
    Offer,     // a one-way button shown while the setting is Off (Defer GPU wait)
};

// Which classes see a row (hidden in game for others; all show when the class
// is unknown, with a class tag). A weapon in hand also shows it (Bow/Staff/Thrown).
enum ClassMask : uint16_t {
    AllClasses=0,
    Amazon=1<<heroclass::Amazon,Sorceress=1<<heroclass::Sorceress,Necromancer=1<<heroclass::Necromancer,
    Paladin=1<<heroclass::Paladin,Barbarian=1<<heroclass::Barbarian,Druid=1<<heroclass::Druid,Assassin=1<<heroclass::Assassin,
    WithBow=1<<8,WithStaff=1<<9,WithThrown=1<<10,
};

// One named choice: its ini value, its name on the chip (<= ~14 chars) and
// the hint the help card shows while it is pointed at or chosen.
struct Option {
    float value;
    const char* label;
    const char* hint;
};
// A word shown instead of a number for one value: { 0, "Off" }.
struct ValueName {
    float value;
    const char* label;
};

// How a number reads: shown = ini / iniScale,
// printed with up to `decimals` (trailing zeros dropped), then the unit. A space
// goes before word units, cm, m and m/s ("65 cm"), none before %, ° and °/s
// ("45%", "30°"). A word unit ending in s drops it for 1 ("1 ring").
struct Format {
    const char* unit="";        // "cm", "°", "%", "°/s", "m/s", "m", "" ...
    float iniScale=1;           // ini value = shown value x iniScale (Waist: .01)
    uint8_t decimals=0;         // the most the step needs; "12 m/s", "12.25 m/s"
    bool sign=false;            // signed ranges show "+"
    bool hueNames=false;        // a hue: swatch + color name ("Azure"), never a number
};

// One row of the menu: one setting (exactly one Key) on one page.
struct ContentRow {
    Key key;                    // the setting; its ini section/key and range: Definitions[key]
    View page;                  // the page it is on
    uint8_t section;            // index of its section within that page (SectionTable order)
    RowKind kind;
    const char* label;          // <= 26 chars
    const char* help;           // <= 160 chars, the card's body
    const char* warn=nullptr;   // amber line under the label where a choice can hurt; nullptr none
    const Option* options=nullptr;   // Choice / Cycler: every option in order
    uint8_t optionCount=0;
    Format format{};            // Stepper / Slider / Swatch
    const ValueName* names=nullptr;  // words for some values ({0,"Off"}), checked before the number
    uint8_t nameCount=0;
    uint16_t classes=AllClasses;     // ClassMask bits; AllClasses: everyone
    float recommended=std::numeric_limits<float>::quiet_NaN();   // ini value where advice differs from the default; NaN none
    bool wraps=false;           // steps wrap round (hues, Lock on); otherwise they stop at the ends
    const char* restartNote=nullptr;  // shown once the applied value differs from the running one (ReShade depth)
};

// A section: 2-5 rows that share an object, titled in sentence case.
struct ContentSection {
    View page;
    const char* title;
};

// A view: a settings page, Presets, Guide or What's new.
struct ContentPage {
    View id;
    const char* name;           // the rail tab / band title: one word for settings pages
    Group group;
    Icon icon;
    const char* intro;          // one line: the page's purpose (the card's idle text)
    bool settings;              // has rows (false: Presets, Guide, WhatsNew)
};

// A preset changes only the keys it lists, in ini units; every other setting
// stays as it is. Rows hidden for the current class are skipped (and said so).
struct PresetValue {
    Key key;
    float value;
};
struct ContentPreset {
    const char* id;
    const char* name;
    Icon icon;
    const char* blurb;
    const PresetValue* values;
    uint8_t count;
};

// Footer and tool buttons: their words and help, keyed by what they do.
struct ContentTool {
    Kind kind;
    const char* label;
    const char* help;
};

// One line of the Controller guide: the control and what it does, in one of
// three columns. Button names follow the Button names setting at paint time:
// GuideControl replaces {R1}-{R4} / {L1}-{L4} with controllers::Labels::Button
// (hand 0 right / 1 left; 1 primary face, 2 secondary, 3 and 4 the same with Grip).
enum class GuideColumn : uint8_t { Left, Right, Panels };
struct GuideLine {
    const char* control;
    const char* action;
    GuideColumn column=GuideColumn::Right;
};

// ---- pages ----

inline constexpr ContentPage PageTable[]={
    {View::Presets,"Presets",Group::Tools,Icon::Presets,"Each preset sets only the rows it lists. Point at one to see what it changes.",false},
    {View::Body,"Body",Group::You,Icon::Body,"Fit the mod to your hands, turning and play space.",true},
    {View::Combat,"Combat",Group::Fight,Icon::Combat,"How your real swings, stabs and throws hit.",true},
    {View::Aim,"Aim",Group::Fight,Icon::Aim,"Where your spells and skills go. Rows for other classes stay hidden.",true},
    {View::Targets,"Targets",Group::Fight,Icon::Targets,"What you see of your aim. Nothing here changes where a skill goes.",true},
    {View::Shield,"Shield",Group::Fight,Icon::Shield,"Bash, block and seat a shield on your arm.",true},
    {View::Wrist,"Wrist",Group::Info,Icon::Wrist,"What your left hand shows when you look at it.",true},
    {View::Interface,"Interface",Group::Info,Icon::Interface,"Game menus, labels and what the monitor shows.",true},
    {View::Survival,"Survival",Group::World,Icon::Survival,"Warnings when you are hurt, and how tough offline monsters are.",true},
    {View::World,"World",Group::World,Icon::World,"Extra sights and moving things the mod adds to the game world.",true},
    {View::Vibration,"Vibration",Group::Device,Icon::Haptics,"Controller vibration. Feel it uses the strength shown here, before you apply.",true},
    {View::Performance,"Performance",Group::Device,Icon::Performance,"Everything that changes the frame rate, with the last second's frame times below.",true},
    {View::Guide,"Guide",Group::Header,Icon::Controls,"Each stick opens its hand's wheel. Point the other hand's laser at a wedge to assign a skill.",false},
    {View::WhatsNew,"What's new",Group::Header,Icon::Notes,"Changes in this version of the mod.",false},
};
static_assert(std::size(PageTable)==size_t(View::Count),"one page per view, in View order");

// Every settings page's sections, in page order; a row's `section` counts
// within its page.
inline constexpr ContentSection SectionTable[]={
    {View::Body,"Hands and turning"},{View::Body,"Potion belt"},{View::Body,"Measurements"},
    {View::Combat,"Melee"},{View::Combat,"Skills and throws"},
    {View::Aim,"Spells and skills"},{View::Aim,"Ground skills"},
    {View::Targets,"Monster outline"},{View::Targets,"Bow"},
    {View::Shield,"Bash and block"},{View::Shield,"Fit on the arm"},
    {View::Wrist,"Life and mana"},{View::Wrist,"Map"},
    {View::Interface,"Game menus"},{View::Interface,"Labels and monitor"},
    {View::Survival,"Low life"},{View::Survival,"Offline monsters"},
    {View::World,"Sky and dungeons"},{View::World,"Things that move and glow"},
    {View::Vibration,"Strength"},
    {View::Performance,"Image"},{View::Performance,"Distance and depth"},
};

// ---- options and value words (ini order: option i has value min + i) ----

inline constexpr Option DominantOptions[]={
    {0,"Right","Reach your right hand over a shoulder and grip to swap weapon sets."},
    {1,"Left","Reach your left hand over a shoulder and grip to swap weapon sets."},
};
inline constexpr Option MeleeRulesOptions[]={
    {0,"Unleashed","Every stroke hits what it reaches. Chance to hit scales damage instead of missing. Adds blocks and parries."},
    {1,"Classic","The game's rules: each stroke rolls to hit one target, and attack speed limits how often."},
};
// Skills go to the aimed monster, spell_lock bits: 1 spells, 2 bow skills.
inline constexpr Option SpellLockOptions[]={
    {0,"Off","Spells and bow skills fly along your casting hand."},
    {1,"Spells","Spells go to the monster you aim at; bow skills fly along your hand."},
    {2,"Bow skills","Bow skills from a button go to the aimed monster; spells fly along your hand."},
    {3,"Both","Spells and bow skills go to the monster you aim at."},
};
// A class without bow skills sees Off / On, which flips the spell bit and keeps
// a saved bow-skills bit (LockStep): match these by the spell bit (SpellLockOption).
inline constexpr Option SpellLockSpellsOnly[]={
    {0,"Off","Spells fly along your casting hand."},
    {1,"On","Spells go to the monster you aim at."},
};
inline constexpr Option AimAssistOptions[]={
    {0,"Off","Spells fly exactly where your hand aims."},
    {1,"Light","A launch within 4\xc2\xb0 of a monster turns toward it."},
    {2,"Strong","A launch within 12\xc2\xb0 turns to the monster, and the spell gently homes in. Charged Bolt gets the launch turn only."},
};
inline constexpr Option GroundAimOptions[]={
    {0,"Game's aim","The game places ground skills its own way."},
    {1,"Pointed","On the floor where your casting hand points, with a landing circle."},
    {2,"Monster","On the monster you aim at, else where you point, with a landing circle."},
};
inline constexpr Option HighlightsOptions[]={
    {0,"Main hand only","One outline: your dominant hand's monster, else the one you look at."},
    {1,"Both hands","Each hand's monster gets its own outline in its own color."},
};
inline constexpr Option StrainDarkOptions[]={
    {0,"Pale glow","A pale glow at the edges of your view."},
    {1,"Dark edge","A dark shadow at the edges of your view."},
    {2,"Off","Nothing shown. Strain still builds and can break your guard."},
};
inline constexpr Option StrainSpeedOptions[]={
    {0,"Slow","About 20 blocks to full strain."},
    {1,"Medium","About 10 blocks to full strain."},
    {2,"Fast","About 5 blocks to full strain."},
};
inline constexpr Option VitalsLocationOptions[]={
    {0,"Wrist","Turn your left wrist toward your face, like checking a watch."},
    {1,"Palm","Raise your open left palm toward you, grip and trigger released."},
    {2,"Wrist or palm","Either gesture shows it."},
    {3,"Off","Not shown on your hand."},
};
inline constexpr Option MapViewOptions[]={
    {0,"Game's view","The game's own slanted map, with NPC and exit names."},
    {1,"Top-down","Rooms laid flat like a floor plan. NPC and exit names are hidden."},
};
inline constexpr Option MenuDistanceOptions[]={
    {0,"Near","Menus 55 cm away, the main menu 70 cm."},
    {1,"Medium","Menus 85 cm away, the main menu 1 m."},
    {2,"Far","Menus 1.15 m away, the main menu 1.3 m."},
};
inline constexpr Option TooltipOptions[]={
    {0,"Instant","Details show as soon as the laser reaches an item."},
    {1,"Short","Details show after the laser rests on an item for 0.3 s."},
    {2,"Game's timing","Details use the game's own delay."},
};
inline constexpr Option ButtonNamesOptions[]={
    {0,"Auto","Uses the names of the controllers you are holding."},
    {1,"Quest","Meta Quest Touch names, whatever is detected."},
    {2,"Index","Valve Index names, whatever is detected."},
    {3,"Steam Frame","Steam Frame names, whatever is detected."},
    {4,"Generic","Plain names that fit any controller."},
};
inline constexpr Option StereoOptions[]={
    {0,"Both","Each frame drawn twice, once per eye. True 3D, the most work."},
    {1,"One by turns","One eye a frame, taking turns. About half the work; fast monsters can look doubled."},
};
inline constexpr Option ResolutionOptions[]={
    {0,"Monitor size","Each eye uses the game window's size."},
    {1,"Headset size","Each eye uses the size your headset asks for, up to 4096."},
    {2,"1536 x 1536","A fixed size per eye. Larger is sharper and costs more."},
    {3,"2048 x 2048","A fixed size per eye. Larger is sharper and costs more."},
    {4,"2560 x 2560","A fixed size per eye. Larger is sharper and costs more."},
    {5,"3072 x 3072","A fixed size per eye. Larger is sharper and costs more."},
    {6,"4096 x 4096","A fixed size per eye. Larger is sharper and costs more."},
};
inline constexpr Option FoveationOptions[]={
    {0,"Off","Full detail everywhere."},
    {1,"Fixed","Less detail toward the lens edges; the center stays sharp."},
    {2,"Eye tracked","The sharp area follows your eyes, on headsets that share eye tracking. Full detail while no gaze arrives."},
};
inline constexpr Option FoveationStrengthOptions[]={
    {0,"Low","Edges barely reduced. The smallest saving."},
    {1,"Medium","A balance of edge detail and speed."},
    {2,"High","The same edge area, coarser."},
    {3,"Maximum","The coarsest edges and a smaller sharp center. The biggest saving."},
};

inline constexpr ValueName OffAtZero[]={{0,"Off"}};
inline constexpr ValueName ByRangeAtZero[]={{0,"By range"}};
inline constexpr ValueName LaserOffName[]={{LaserOff,"Off"}};
inline constexpr ValueName ShieldFollowNames[]={{0,"Forearm"},{100,"Wrist"}};
inline constexpr ValueName OfflinePlayersNames[]={{0,"Game's setting"},{1,"1 player"}};

// The unit strings (° is \xc2\xb0: the build has no /utf-8).
inline constexpr const char* Degrees="\xc2\xb0";
inline constexpr const char* DegreesPerSecond="\xc2\xb0/s";

#define FRAMEVR_OPTIONS(list) .options=list,.optionCount=uint8_t(std::size(list))
#define FRAMEVR_NAMES(list) .names=list,.nameCount=uint8_t(std::size(list))

// ---- rows, page by page, section by section (layout::RowsOf keeps this order) ----

inline constexpr ContentRow RowTable[]={
    // Body: Hands and turning
    {.key=Dominant,.page=View::Body,.section=0,.kind=RowKind::Choice,.label="Dominant hand",
     .help="The hand that swaps weapon sets when you reach over a shoulder and grip. Your weapon and shield hands in the game stay the same.",
     FRAMEVR_OPTIONS(DominantOptions)},
    {.key=SmoothTurn,.page=View::Body,.section=0,.kind=RowKind::Toggle,.label="Smooth turning",
     .help="On turns you smoothly with the right stick. Off turns you in steps, which many players find easier on the stomach."},
    {.key=Snap,.page=View::Body,.section=0,.kind=RowKind::Stepper,.label="Snap turn angle",
     .help="How far each flick of the right stick turns you. Smaller angles give finer steps.",
     .format={.unit=Degrees}},
    {.key=TurnSpeed,.page=View::Body,.section=0,.kind=RowKind::Slider,.label="Smooth turn speed",
     .help="How fast the right stick turns you, in degrees per second. Lower is gentler.",
     .format={.unit=DegreesPerSecond}},
    // Body: Potion belt
    {.key=Seated,.page=View::Body,.section=1,.kind=RowKind::Toggle,.label="Seated play",
     .help="Raises the potion belt so you can reach it sitting down. Your body measurements still come from standing."},
    {.key=Waist,.page=View::Body,.section=1,.kind=RowKind::Slider,.label="Belt below eyes",
     .help="How far below your eyes the potion belt sits when standing. Lower numbers raise the belt.",
     .format={.unit="cm",.iniScale=.01f}},
    {.key=BeltForward,.page=View::Body,.section=1,.kind=RowKind::Slider,.label="Belt forward reach",
     .help="How far in front of your body the belt sits. Raise it if the belt is too close; lower it if you reach too far.",
     .format={.unit="cm",.iniScale=.01f,.decimals=1}},
    // Body: Measurements
    {.key=BodyCalibration,.page=View::Body,.section=2,.kind=RowKind::Button,.label="Height and arm span",
     .help="Stand straight, then hold your arms out in a T. Measures your eye height and arm span so the world and your arms fit you."},

    // Combat: Melee
    {.key=Motion,.page=View::Combat,.section=0,.kind=RowKind::Toggle,.label="Motion combat",
     .help="Swing weapons, draw bows, throw and use the belt with your hands. Keep this On: it is what makes the mod play in VR.",
     .warn="Off turns off swings, bow draw, throws and the belt",.recommended=1},
    {.key=MeleeRules,.page=View::Combat,.section=0,.kind=RowKind::Choice,.label="Melee rules",
     .help="How your real melee strokes deal damage: the mod's Unleashed rules or the game's Classic dice rolls.",
     FRAMEVR_OPTIONS(MeleeRulesOptions)},
    {.key=MeleeHitSpeed,.page=View::Combat,.section=0,.kind=RowKind::Stepper,.label="Swing speed to hit",
     .help="How fast a swing must move to strike. Lower makes hits easier; higher asks for real swings, with no upper limit. Shield bash has its own speed.",
     .format={.unit="m/s",.decimals=2}},
    {.key=MeleeThrustSpeed,.page=View::Combat,.section=0,.kind=RowKind::Slider,.label="Stab speed to hit",
     .help="How fast a forward stab along the blade must move to strike. Pull back before you stab again.",
     .format={.unit="m/s",.decimals=2}},
    // Combat: Skills and throws
    {.key=SkillGestures,.page=View::Combat,.section=1,.kind=RowKind::Toggle,.label="Skills on bash and bow",
     .help="A trigger skill powers your real move: Smite on a shield bash, arrow skills on a bow release. Plain swings stay plain."},
    {.key=WeaponSkillIndicators,.page=View::Combat,.section=1,.kind=RowKind::Toggle,.label="Skill icons on weapons",
     .help="Shows each trigger skill's icon and name beside the weapon that casts it. Turn it On while you learn your setup."},
    {.key=ThrowLift,.page=View::Combat,.section=1,.kind=RowKind::Stepper,.label="Thrown weapon lift",
     .help="Your hand dips as you let go. This lifts javelins, knives and axes above that path. Minus lowers them; Off follows your hand.",
     .format={.unit=Degrees,.sign=true},FRAMEVR_NAMES(OffAtZero),.classes=Amazon|Barbarian|Assassin|WithThrown},

    // Aim: Spells and skills (Aim with your view sits here: it is not a ground skill)
    {.key=SpellLock,.page=View::Aim,.section=0,.kind=RowKind::Cycler,.label="Skills go to the aimed monster",
     .help="Casts go to the monster your hand points at, else the other hand's, else the one you look at, and stay on it until you let go.",
     FRAMEVR_OPTIONS(SpellLockOptions),.wraps=true},
    {.key=ProjectileAimAssist,.page=View::Aim,.section=0,.kind=RowKind::Choice,.label="Spell aim assist",
     .help="Helps supported hand spells find their monster. Bows and crossbows are always aimed by you alone.",
     FRAMEVR_OPTIONS(AimAssistOptions)},
    {.key=PlanarSpellAim,.page=View::Aim,.section=0,.kind=RowKind::Toggle,.label="Keep spell shots level",
     .help="On keeps straight spell shots level with the ground. Off lets you aim them up or down."},
    {.key=StaffPoint,.page=View::Aim,.section=0,.kind=RowKind::Toggle,.label="Aim staff along arm",
     .help="An upright staff casts along your arm, from shoulder through hand. Held out flat like a wand, it casts from its tip as before.",
     .classes=Sorceress|Druid|WithStaff},
    {.key=AimFollowsView,.page=View::Aim,.section=0,.kind=RowKind::Toggle,.label="Aim with your view",
     .help="Only monsters in front of your face can be aimed at. A hand held out and pointing still picks its own monster."},
    // Aim: Ground skills
    {.key=GroundAim,.page=View::Aim,.section=1,.kind=RowKind::Choice,.label="Ground skill aim",
     .help="Where skills like Blizzard, Meteor, Fire Wall, traps and Bone Wall land.",
     FRAMEVR_OPTIONS(GroundAimOptions),.classes=Sorceress|Assassin|Druid|Necromancer},
    {.key=LandingCircles,.page=View::Aim,.section=1,.kind=RowKind::Toggle,.label="Landing circles",
     .help="A circle on the floor in the skill's color shows where it will land. It fills up until the skill hits.",
     .classes=Sorceress|Assassin|Druid|Necromancer},

    // Targets: Monster outline (left before right)
    {.key=TargetHighlight,.page=View::Targets,.section=0,.kind=RowKind::Slider,.label="Monster outline",
     .help="Outlines the monster you aim at, as strongly as the percentage. A hand held out picks its monster; otherwise the one you look at.",
     .format={.unit="%"},FRAMEVR_NAMES(OffAtZero)},
    {.key=AimHighlights,.page=View::Targets,.section=0,.kind=RowKind::Choice,.label="Outlines shown",
     .help="Outline only your dominant hand's monster, or one for each hand.",
     FRAMEVR_OPTIONS(HighlightsOptions)},
    {.key=HighlightLeft,.page=View::Targets,.section=0,.kind=RowKind::Swatch,.label="Left hand outline color",
     .help="The left hand's outline color. By range: red near, orange mid, yellow far. Pick a color unlike the monsters' own.",
     .format={.hueNames=true},FRAMEVR_NAMES(ByRangeAtZero)},
    {.key=HighlightRight,.page=View::Targets,.section=0,.kind=RowKind::Swatch,.label="Right hand outline color",
     .help="The right hand's outline color. By range: red near, orange mid, yellow far. Pick a color unlike the monsters' own.",
     .format={.hueNames=true},FRAMEVR_NAMES(ByRangeAtZero)},
    // Targets: Bow
    {.key=BowLaser,.page=View::Targets,.section=1,.kind=RowKind::Swatch,.label="Arrow path line",
     .help="At full draw, a faint line shows the arrow's flight, up to 15 m, through walls. Not for crossbows, Multiple Shot, Guided Arrow or Strafe.",
     .format={.hueNames=true},FRAMEVR_NAMES(LaserOffName),.classes=Amazon|WithBow,.wraps=true},

    // Shield: Bash and block
    {.key=ShieldBashSpeed,.page=View::Shield,.section=0,.kind=RowKind::Slider,.label="Bash speed to hit",
     .help="How fast your shield hand must move to bash, Smite included. Move at least 12 cm in one stroke; twisting the wrist never bashes.",
     .format={.unit="m/s",.decimals=2}},
    {.key=StrainDark,.page=View::Shield,.section=0,.kind=RowKind::Choice,.label="Guard strain edge",
     .help="In Unleashed, blocking many hits in a row strains your guard. This shows it at the edge of your view. It fades when hits stop.",
     FRAMEVR_OPTIONS(StrainDarkOptions)},
    {.key=StrainSpeed,.page=View::Shield,.section=0,.kind=RowKind::Choice,.label="Guard strain buildup",
     .help="How quickly blocked hits strain your guard. Lowering the shield or a few seconds without hits clears it.",
     FRAMEVR_OPTIONS(StrainSpeedOptions)},
    // Shield: Fit on the arm
    {.key=ShieldGrip,.page=View::Shield,.section=1,.kind=RowKind::Slider,.label="Shield along forearm",
     .help="Slides the shield toward your elbow (+) so your hand reaches its strap, or toward your hand (-). About 3 cm suits a buckler.",
     .format={.unit="cm",.sign=true}},
    {.key=ShieldOut,.page=View::Shield,.section=1,.kind=RowKind::Slider,.label="Shield out from arm",
     .help="Moves the shield away from your forearm (+) or into it (-). It blocks where you see it.",
     .format={.unit="cm",.sign=true}},
    {.key=ShieldUp,.page=View::Shield,.section=1,.kind=RowKind::Slider,.label="Shield across forearm",
     .help="Moves the shield sideways across your forearm. Use it with the two rows above to seat the shield on your arm.",
     .format={.unit="cm",.sign=true}},
    {.key=ShieldFollow,.page=View::Shield,.section=1,.kind=RowKind::Slider,.label="Shield wrist turn",
     .help="0% straps the shield to your forearm, so bending the wrist leaves it still. 100% turns it with your hand.",
     .format={.unit="%"},FRAMEVR_NAMES(ShieldFollowNames)},
    {.key=ShieldRoll,.page=View::Shield,.section=1,.kind=RowKind::Slider,.label="Shield roll",
     .help="Turns the shield around your forearm; + turns it toward your little finger. +90\xc2\xb0 suits a controller held palm down.",
     .format={.unit=Degrees,.sign=true}},

    // Wrist: Life and mana
    {.key=VitalsLocation,.page=View::Wrist,.section=0,.kind=RowKind::Choice,.label="Life and mana",
     .help="Which left-hand gesture shows your health and mana.",
     FRAMEVR_OPTIONS(VitalsLocationOptions)},
    {.key=VitalsTilt,.page=View::Wrist,.section=0,.kind=RowKind::Slider,.label="Life and mana tilt",
     .help="Tilts the health and mana display on your hand; + tips its top toward you. The minimap keeps its own angle.",
     .format={.unit=Degrees,.sign=true}},
    // Wrist: Map
    {.key=NavigationLocation,.page=View::Wrist,.section=1,.kind=RowKind::Choice,.label="Map and compass",
     .help="Which left-hand gesture shows the minimap and compass, separately from health and mana.",
     FRAMEVR_OPTIONS(VitalsLocationOptions)},
    {.key=MapTopDown,.page=View::Wrist,.section=1,.kind=RowKind::Choice,.label="Map view",
     .help="How the wrist minimap is drawn. The gold arrow shows which way you face.",
     FRAMEVR_OPTIONS(MapViewOptions)},
    {.key=MapRotate,.page=View::Wrist,.section=1,.kind=RowKind::Toggle,.label="Turn map with you",
     .help="On keeps the way you face at the top of the map. Off keeps north at the top and turns the arrow. Top-down view only."},
    {.key=CompanionCounts,.page=View::Wrist,.section=1,.kind=RowKind::Toggle,.label="Buffs and companions",
     .help="Under the minimap: your companion counts, and the buffs, auras and harmful effects on you. Follows the minimap's gesture."},

    // Interface: Game menus
    {.key=MenuDistance,.page=View::Interface,.section=0,.kind=RowKind::Choice,.label="Menu distance",
     .help="How far away the inventory, merchants and main menu open. Point at one, hold Right Grip and push or pull to move it.",
     FRAMEVR_OPTIONS(MenuDistanceOptions)},
    {.key=TooltipTiming,.page=View::Interface,.section=0,.kind=RowKind::Choice,.label="Item details delay",
     .help="How soon an item's details pop up when you point the laser at it in a menu.",
     FRAMEVR_OPTIONS(TooltipOptions)},
    {.key=ControllerLabels,.page=View::Interface,.section=0,.kind=RowKind::Cycler,.label="Button names",
     .help="Which controller's button names appear in help and menus. Only the names change, not what the buttons do.",
     FRAMEVR_OPTIONS(ButtonNamesOptions)},
    // Interface: Labels and monitor
    {.key=LootLabelSize,.page=View::Interface,.section=1,.kind=RowKind::Slider,.label="Loot name size",
     .help="Size of item names on the ground; 45% is the game's size. Hold the right stick down to show every nearby name.",
     .format={.unit="%"}},
    {.key=OrdinaryMobBars,.page=View::Interface,.section=1,.kind=RowKind::Toggle,.label="Weak monster bars",
     .help="Names and health bars over ordinary monsters. Bosses and elites always show theirs. Off keeps the view clear."},
    {.key=DesktopOneEye,.page=View::Interface,.section=1,.kind=RowKind::Toggle,.label="One eye on monitor",
     .help="On shows a steady left-eye image on your monitor, good for streaming. Off saves a little graphics work. The headset is the same."},

    // Survival: Low life
    {.key=CriticalAlerts,.page=View::Survival,.section=0,.kind=RowKind::Toggle,.label="Low life and mana alert",
     .help="A brief warning and a buzz when health or mana runs low. Set the buzz under Vibration > Low life and mana alerts."},
    {.key=HeartbeatStart,.page=View::Survival,.section=0,.kind=RowKind::Stepper,.label="Heartbeat starts at",
     .help="The life level where a heartbeat starts. It grows louder and faster with every 10% you lose. Off silences it.",
     .format={.unit="%"},FRAMEVR_NAMES(OffAtZero)},
    {.key=BloodVisor,.page=View::Survival,.section=0,.kind=RowKind::Toggle,.label="Blood on visor",
     .help="Blood splashes the edges of your view as life drops below 60%, 40% and 20%. Healing back up fades it."},
    {.key=BloodWipe,.page=View::Survival,.section=0,.kind=RowKind::Toggle,.label="Wipe blood by hand",
     .help="Wave a hand side to side in front of your face, trigger released, to wipe the blood away. It does not heal you."},
    // Survival: Offline monsters
    {.key=OfflinePlayers,.page=View::Survival,.section=1,.kind=RowKind::Stepper,.label="Offline difficulty",
     .help="Monsters as tough as in a game of this many players, with more experience and loot. Applies to monsters that spawn after Apply.",
     .format={.unit="players"},FRAMEVR_NAMES(OfflinePlayersNames)},
    {.key=EnemySight,.page=View::Survival,.section=1,.kind=RowKind::Slider,.label="Monster sight range",
     .help="How far away monsters notice you offline. 100% is the game's range; 300% is three times as far.",
     .format={.unit="%"}},
    {.key=CompanionAi,.page=View::Survival,.section=1,.kind=RowKind::Toggle,.label="Smarter mercenary",
     .help="Your mercenary keeps close in fights, spots monsters as far as they spot you, and is moved behind you if it gets stuck."},

    // World: Sky and dungeons
    {.key=Sky,.page=View::World,.section=0,.kind=RowKind::Toggle,.label="Outdoor sky",
     .help="Adds a sky above outdoor areas. It does not show more of the map."},
    {.key=Fog,.page=View::World,.section=0,.kind=RowKind::Toggle,.label="Distance fog",
     .help="Adds haze to distant outdoor scenery. Off makes far things sharper."},
    {.key=Ceiling,.page=View::World,.section=0,.kind=RowKind::Toggle,.label="Dungeon ceilings",
     .help="Covers the open black space above you in supported dungeons. Building roofs are the game's own."},
    // World: Things that move and glow
    {.key=BottlePhysics,.page=View::World,.section=1,.kind=RowKind::Toggle,.label="Roll and break bottles",
     .help="Empty bottles drop and roll. Potions thrown at your mercenary break where they hit; thrown traps stop at walls."},
    {.key=PropPhysics,.page=View::World,.section=1,.kind=RowKind::Toggle,.label="Knock over small props",
     .help="Cups, vases, books and stools tumble when your weapon, hand or shield hits them. Just for looks."},
    {.key=ElementalHands,.page=View::World,.section=1,.kind=RowKind::Toggle,.label="Spell glow on hands",
     .help="A Sorceress's hands show flames, sparks or frost from their trigger spell, and flare as it casts.",
     .warn="May show through walls and weapons",.classes=Sorceress},

    // Vibration: Strength (Feel it: haptics::PreviewLabel names each test)
    {.key=HapticStrength,.page=View::Vibration,.section=0,.kind=RowKind::Slider,.label="Overall vibration",
     .help="Scales every kind of vibration below on both controllers. 100% is normal; Off stops all vibration.",
     .format={.unit="%"},FRAMEVR_NAMES(OffAtZero)},
    {.key=HapticMelee,.page=View::Vibration,.section=0,.kind=RowKind::Slider,.label="Melee hits",
     .help="Weapon and shield impacts. Feel it: Weapon buzzes the right controller, Shield the left.",
     .format={.unit="%"},FRAMEVR_NAMES(OffAtZero)},
    {.key=HapticShortMelee,.page=View::Vibration,.section=0,.kind=RowKind::Toggle,.label="Short melee taps",
     .help="On uses very short, sharp taps for hits. Some controllers barely feel them: leave it Off if hits feel weak."},
    {.key=HapticBowDraw,.page=View::Vibration,.section=0,.kind=RowKind::Slider,.label="Bow draw",
     .help="Nocking an arrow, pulling the string and reaching full draw.",
     .format={.unit="%"},FRAMEVR_NAMES(OffAtZero)},
    {.key=HapticBowRelease,.page=View::Vibration,.section=0,.kind=RowKind::Slider,.label="Bow and throw release",
     .help="Letting an arrow fly or throwing a javelin.",
     .format={.unit="%"},FRAMEVR_NAMES(OffAtZero)},
    {.key=HapticSpell,.page=View::Vibration,.section=0,.kind=RowKind::Slider,.label="Spell casts",
     .help="One short buzz in the casting hand each time a supported hand spell casts.",
     .format={.unit="%"},FRAMEVR_NAMES(OffAtZero)},
    {.key=HapticInteraction,.page=View::Vibration,.section=0,.kind=RowKind::Slider,.label="Loot, potions and menus",
     .help="Picking up loot, handling potions, holsters and pressing menu buttons.",
     .format={.unit="%"},FRAMEVR_NAMES(OffAtZero)},
    {.key=HapticAlert,.page=View::Vibration,.section=0,.kind=RowKind::Slider,.label="Low life and mana alerts",
     .help="The low health and mana buzz on the left controller. Feel it works anyway; real alerts also need Survival > Low life and mana alert On.",
     .format={.unit="%"},FRAMEVR_NAMES(OffAtZero)},

    // Performance: Image
    {.key=StereoRendering,.page=View::Performance,.section=0,.kind=RowKind::Choice,.label="Eyes drawn per frame",
     .help="The biggest speed setting: drawing one eye a frame can nearly double the frame rate. Menus and views other than first person draw both eyes.",
     FRAMEVR_OPTIONS(StereoOptions)},
    {.key=RenderResolution,.page=View::Performance,.section=0,.kind=RowKind::Cycler,.label="Render resolution",
     .help="The image size for each eye. Higher is sharper but costs graphics card time. DLSS is in the game's own Options.",
     FRAMEVR_OPTIONS(ResolutionOptions)},
    {.key=Foveation,.page=View::Performance,.section=0,.kind=RowKind::Choice,.label="Lower detail at edges",
     .help="Draws the edges of the view, where you see least, in less detail to save graphics card time.",
     FRAMEVR_OPTIONS(FoveationOptions)},
    {.key=FoveationStrength,.page=View::Performance,.section=0,.kind=RowKind::Choice,.label="Edge detail reduction",
     .help="How much detail the edges lose. Stronger saves more time but is easier to notice.",
     FRAMEVR_OPTIONS(FoveationStrengthOptions)},
    // Performance: Distance and depth
    {.key=DrawDistance,.page=View::Performance,.section=1,.kind=RowKind::Stepper,.label="Terrain distance",
     .help="How many rings of map tiles stay drawn around you. Lower it first if big outdoor areas run slowly.",
     .format={.unit="rings"}},
    {.key=ObjectDistance,.page=View::Performance,.section=1,.kind=RowKind::Slider,.label="Scenery distance",
     .help="How far away buildings and props stay visible. The default ends just past the fog. Lower helps busy areas such as Act III."},
    {.key=ReShadeDepth,.page=View::Performance,.section=1,.kind=RowKind::Toggle,.label="ReShade depth add-on",
     .help="In VR the mod gives fog and sky their own depth. Off saves work, but the flat menu and play outside VR lose fog and sky depth. Applies after a game restart.",
     .restartNote=DepthRestartNote},
    {.key=DeferGpuWait,.page=View::Performance,.section=1,.kind=RowKind::Offer,.label="Defer GPU wait",
     .help="Starts the next frame while the graphics card finishes the last; without it, about half the frame rate. Once on, the menu cannot turn it off.",
     .warn=DeferWarning,.recommended=1},
};
#undef FRAMEVR_OPTIONS
#undef FRAMEVR_NAMES

// Settings with an ini key but no row (set by Calibrate body, or legacy).
inline constexpr Key NotInMenu[]={EyeHeight,ArmSpan,WristMap,WristCompass};

// ---- presets (ini units; only the keys listed change) ----
// Left out of the presets: StereoRendering (Comfort, Smooth frames), DeferGpuWait
// (Smooth frames), ElementalHands and CompanionAi (Classic, Full VR), and the
// PR-only LootPick / MenuPointer.

inline constexpr PresetValue ComfortValues[]={
    {SmoothTurn,0},{Snap,30},
    {Motion,1},{MeleeHitSpeed,8},
    {ProjectileAimAssist,2},{SpellLock,3},{GroundAim,2},{LandingCircles,1},
    {TargetHighlight,50},{AimHighlights,1},
    {MenuDistance,1},{TooltipTiming,0},{LootLabelSize,60},
    {BloodVisor,0},{HeartbeatStart,0},{CriticalAlerts,1},
};
inline constexpr PresetValue ClassicValues[]={
    {Motion,1},{MeleeRules,1},
    {ProjectileAimAssist,0},{SpellLock,0},{PlanarSpellAim,0},{GroundAim,0},{TargetHighlight,0},
    {TooltipTiming,2},{LootLabelSize,45},{OrdinaryMobBars,1},
    {OfflinePlayers,0},{EnemySight,100},
    {BloodVisor,0},{HeartbeatStart,0},
    {Sky,0},{Fog,0},{Ceiling,0},{BottlePhysics,0},{PropPhysics,0},
    {MapTopDown,0},
};
inline constexpr PresetValue FullVrValues[]={
    {Motion,1},{MeleeRules,0},{SkillGestures,1},{WeaponSkillIndicators,1},
    {SmoothTurn,1},{TurnSpeed,120},
    {SpellLock,3},{ProjectileAimAssist,1},{StaffPoint,1},{GroundAim,1},{LandingCircles,1},
    {TargetHighlight,60},{AimHighlights,1},{BowLaser,45},
    {BloodVisor,1},{BloodWipe,1},{HeartbeatStart,60},{CriticalAlerts,1},
    {Sky,1},{Fog,1},{Ceiling,1},{BottlePhysics,1},{PropPhysics,1},
    {CompanionCounts,1},
};
inline constexpr PresetValue SmoothFramesValues[]={
    {RenderResolution,1},
    {Foveation,1},{FoveationStrength,2},
    {DrawDistance,2},{ObjectDistance,210},
    {DesktopOneEye,0},
    {Sky,0},{Fog,0},
};
inline constexpr ContentPreset PresetTable[]={
    {"comfort","Comfort",Icon::Comfort,"Gentle turns, strong aim help, no blood or heartbeat.",ComfortValues,uint8_t(std::size(ComfortValues))},
    {"classic","Classic",Icon::Classic,"The game's own rules and looks, with only what VR needs.",ClassicValues,uint8_t(std::size(ClassicValues))},
    {"fullvr","Full VR",Icon::FullVr,"Everything physical and visible: the mod as it was meant to feel.",FullVrValues,uint8_t(std::size(FullVrValues))},
    {"fast","Smooth frames",Icon::Fast,"Less to draw, for older PCs: headset resolution, coarser edges, short distances, no sky or fog.",SmoothFramesValues,uint8_t(std::size(SmoothFramesValues))},
};

// ---- tools: footer, confirm row, card buttons ----

inline constexpr ContentTool ToolTable[]={
    {Kind::Apply,"Apply","Saves and uses your changes. Until you press it, changes are only a draft. Your other settings and calibration stay as they are."},
    {Kind::Close,"Close","Back to the game. If some changes are not applied, Close asks first."},
    {Kind::Recenter,"Set eye height","Sets your eye height for where you stand or sit now. Your body measurements stay saved."},
    {Kind::Calibrate,"Calibrate body","Measures your standing eye height and arm span in a T-pose. Apply or discard changes first. Hold both triggers when asked; Right B cancels."},
    {Kind::ResetPage,"Reset page","Sets every row you can see on this page back to its default. Nothing is saved until you press Apply."},
    {Kind::UndoReset,"Undo reset","Puts back the values this page had before you pressed Reset page. It goes away after 6 seconds."},
    // The confirm row; the layout may print the count ("Discard 3 changes").
    {Kind::ConfirmClose,"Discard changes","Closes the menu and puts every changed row back to its applied value."},
    {Kind::KeepEditing,"Keep editing","Back to the page. Your changes stay as a draft."},
    {Kind::ApplyClose,"Apply and close","Saves your changes, then closes once they are saved."},
    // The help card's buttons.
    {Kind::Undo,"Undo","Puts this row back to its applied value."},
    {Kind::Default,"Default","Puts this row back to its default."},
    {Kind::TestHaptics,"Feel it","Vibrates at the strength shown here, before you apply."},
    {Kind::Open,"Open","Goes to the row or page this one needs."},
    {Kind::Preset,"Use preset","Sets only the rows this preset lists. Nothing is saved until you press Apply."},
};

// ---- the Controller guide (main's Controls page, vr_settings_ui.cpp) ----

inline constexpr GuideLine GuideTable[]={
    {"Stick","Walk",GuideColumn::Left},
    {"{L1} or {L2}","Your left-hand skills",GuideColumn::Left},
    {"{L3} or {L4}","More left-hand skills",GuideColumn::Left},
    {"Trigger","Left-hand skill",GuideColumn::Left},
    {"Stick click","Open or close the left wheel",GuideColumn::Left},
    {"Grip + stick click","Inventory, character and skills",GuideColumn::Left},
    {"Grip near the belt","Grab a potion",GuideColumn::Left},
    {"Grip + right trigger","Right-click in game menus",GuideColumn::Left},
    {"Stick left or right","Turn",GuideColumn::Right},
    {"Stick click","Open or close the right wheel",GuideColumn::Right},
    {"Trigger","Right-hand skill, or grab a bow string",GuideColumn::Right},
    {"Stick up","Run or walk",GuideColumn::Right},
    {"Hold stick down","Show all loot names",GuideColumn::Right},
    {"{R1} or {R2}","Interact, or your skills",GuideColumn::Right},
    {"{R3} or {R4}","More right-hand skills",GuideColumn::Right},
    {"Main-hand grip at a shoulder","Swap weapon sets",GuideColumn::Right},
    {"Trigger with bow / javelin","Draw, release, throw",GuideColumn::Right},
    {"Left Trigger","Ctrl in game menus",GuideColumn::Panels},
    {"Right Grip","Shift; push or pull a menu",GuideColumn::Panels},
    {"Right {R2}","Back",GuideColumn::Panels},
};
// What's new: the existing release_notes.h (releasenotes::), unchanged.

// ---- status lines (main's words, vr_settings_ui.cpp) ----

// Under Offline difficulty: the card leads with the note; without the
// offline scaling adapter, main's notice instead (row and card), and the row
// stays editable as on main.
inline constexpr const char* OfflineNote="Difficulty affects new monsters only; existing monsters keep their stats. To apply it throughout the game, Save and Exit, then start a new game with the same character.";
inline constexpr const char* OfflineUnavailable="Offline scaling adapter unavailable. Game difficulty remains unchanged.";
inline constexpr const char* OfflineUnavailableShort="Offline scaling adapter unavailable";
// Under Button names: the controllers the headset reports.
inline std::string DetectedText(const controllers::Labels& detected){
    return std::string("Detected: left ")+controllers::Name(detected.hands[1])+" / right "+controllers::Name(detected.hands[0]);
}

// ---- color names: one per 15-degree hue step (the swatch rows and the grid) ----

inline constexpr const char* HueNames[24]={
    "Red","Vermilion","Orange","Gold","Yellow","Lime","Chartreuse","Leaf","Green","Jade","Spring","Mint",
    "Cyan","Sky","Azure","Cobalt","Blue","Indigo","Violet","Purple","Magenta","Pink","Rose","Crimson"};
inline const char* HueNameOf(float hue){
    const int step=int(std::lround(hue/15.f));
    return HueNames[((step%24)+24)%24];
}

// ---- lookups (trivial over the tables; the checks below and the tests check the tables) ----

inline const ContentPage& PageOf(View v){return PageTable[size_t(v)];}
// The row of a key, or nullptr (NotInMenu).
inline const ContentRow* RowOf(Key key){
    for(const auto& r:RowTable)if(r.key==key)return &r;
    return nullptr;
}
// Whether a row is for this character: a class bit or the weapon in hand.
// Unknown class (-1): every row.
inline bool ForCharacter(const ContentRow& r,const Context& c){
    if(r.classes==AllClasses||c.heroClass<0)return true;
    if(c.heroClass<8&&(r.classes&(1u<<c.heroClass)))return true;
    return (c.bow&&(r.classes&WithBow))||(c.staff&&(r.classes&WithStaff))||(c.thrown&&(r.classes&WithThrown));
}
// The row's label for this character: Skills go to the aimed monster reads
// "Spells go to the aimed monster" for a class without bow skills (RowLabel).
inline const char* LabelFor(const ContentRow& r,const Context& c={}){
    return r.key==SpellLock&&!BowSkillClass(c)?"Spells go to the aimed monster":r.label;
}
// Whether this character sees the spells-only Off / On of Skills go to the aimed monster.
inline bool SpellsOnly(const ContentRow& r,const Context& c){return r.key==SpellLock&&!BowSkillClass(c);}
// The options this character steps through (count in `count`).
inline const Option* OptionsFor(const ContentRow& r,const Context& c,int& count){
    if(SpellsOnly(r,c)){count=int(std::size(SpellLockSpellsOnly));return SpellLockSpellsOnly;}
    count=r.optionCount;return r.options;
}
// The option a value selects, or nullptr.
inline const Option* OptionOf(const ContentRow& r,float value){
    for(int i=0;i<r.optionCount;++i)if(std::fabs(r.options[i].value-value)<.001f)return &r.options[i];
    return nullptr;
}
// The same for this character: the spells-only list matches by the spell bit.
inline const Option* OptionOf(const ContentRow& r,float value,const Context& c){
    if(SpellsOnly(r,c))return &SpellLockSpellsOnly[LockSpells(value)?1:0];
    return OptionOf(r,value);
}
// The value as the row shows it ("65 cm", "Off", "Strong", "+10°", "Azure", "3 players").
inline std::string ValueText(const ContentRow& r,float value,const Context& c={}){
    if(!std::isfinite(value))return "";
    if(const Option* o=OptionOf(r,value,c))return o->label;
    for(int i=0;i<r.nameCount;++i)if(std::fabs(r.names[i].value-value)<.001f)return r.names[i].label;
    if(r.key==BowLaser&&value<0)return "Off";   // any negative hue is Off (Normalize)
    if(r.kind==RowKind::Toggle||r.kind==RowKind::Offer)return value>.5f?"On":"Off";
    if(r.format.hueNames)return HueNameOf(value);
    const float scale=r.format.iniScale>0?r.format.iniScale:1.f;
    const double shown=double(value)/double(scale)+0.;
    char number[48];
    std::snprintf(number,sizeof number,"%.*f",int(r.format.decimals),shown);
    std::string text=number;
    if(text.find('.')!=std::string::npos){   // no needless decimals: 12.50 -> 12.5, 12.00 -> 12
        while(!text.empty()&&text.back()=='0')text.pop_back();
        if(!text.empty()&&text.back()=='.')text.pop_back();
    }
    if(text=="-0")text="0";
    if(r.format.sign&&text!="0"&&text[0]!='-')text="+"+text;
    const std::string unit=r.format.unit?r.format.unit:"";
    if(unit.empty())return text;
    if(unit[0]=='%'||unit[0]=='\xc2')return text+unit;   // %, °, °/s: no space
    bool word=true;
    for(char ch:unit)if(ch<'a'||ch>'z'){word=false;break;}
    if(word&&unit.size()>1&&unit.back()=='s'&&(text=="1"||text=="+1"||text=="-1"))return text+" "+unit.substr(0,unit.size()-1);
    return text+" "+unit;
}
inline const ContentTool* ToolOf(Kind kind){
    for(const auto& t:ToolTable)if(t.kind==kind)return &t;
    return nullptr;
}
// A guide line's control with this player's button names.
inline std::string GuideControl(const GuideLine& g,const controllers::Labels& labels){
    std::string out;
    for(const char* p=g.control;*p;){
        if(p[0]=='{'&&(p[1]=='L'||p[1]=='R')&&p[2]>='1'&&p[2]<='4'&&p[3]=='}'){
            out+=labels.Button(p[1]=='L'?1:0,p[2]-'0');p+=4;
        }else out+=*p++;
    }
    return out;
}

// ---- compile-time checks of the tables ----
namespace check {
constexpr size_t Length(const char* s){size_t n=0;while(s&&s[n])++n;return n;}
// Every Key except NotInMenu is in exactly one row; NotInMenu keys in none.
constexpr bool EveryKeyOnce(){
    for(int k=0;k<Count;++k){
        int n=0;
        for(const auto& r:RowTable)if(r.key==k)++n;
        for(Key m:NotInMenu)if(m==k)++n;
        if(n!=1)return false;
    }
    return true;
}
constexpr bool IsChoiceKind(RowKind k){return k==RowKind::Choice||k==RowKind::Cycler;}
// Lengths: labels 26 (the one long main label 30), help and hints 160, warn 56, option names 14.
constexpr bool TextsFit(){
    for(const auto& r:RowTable){
        if(!Length(r.label)||Length(r.label)>(r.key==SpellLock?30u:26u))return false;
        if(!Length(r.help)||Length(r.help)>160)return false;
        if(r.warn&&Length(r.warn)>56)return false;
        if(r.restartNote&&Length(r.restartNote)>60)return false;
        for(int i=0;i<r.optionCount;++i)
            if(!Length(r.options[i].label)||Length(r.options[i].label)>14||!Length(r.options[i].hint)||Length(r.options[i].hint)>160)return false;
        for(int i=0;i<r.nameCount;++i)if(!Length(r.names[i].label)||Length(r.names[i].label)>14)return false;
    }
    for(const auto& p:PageTable)if(!Length(p.name)||Length(p.intro)>160)return false;
    for(const auto& s:SectionTable)if(!Length(s.title)||Length(s.title)>26)return false;
    for(const auto& t:ToolTable)if(!Length(t.label)||Length(t.label)>20||!Length(t.help)||Length(t.help)>160)return false;
    for(const auto& p:PresetTable)if(!Length(p.name)||Length(p.name)>14||!Length(p.blurb)||Length(p.blurb)>100)return false;
    for(const auto& o:SpellLockSpellsOnly)if(Length(o.label)>14||Length(o.hint)>160)return false;
    for(const auto& g:GuideTable)if(!Length(g.control)||Length(g.control)>30||!Length(g.action)||Length(g.action)>40)return false;
    return true;
}
// Each kind fits its setting: a toggle or offer is 0/1; a choice lists every
// ini value from min in order (ini order); a slider has an upper limit; a
// swatch names its hues; a stepper with no upper limit shows no bar.
constexpr bool KindsMatch(){
    for(const auto& r:RowTable){
        const auto& d=Definitions[r.key];
        const bool open=d.max>=std::numeric_limits<float>::max();
        switch(r.kind){
            case RowKind::Toggle:case RowKind::Offer:case RowKind::Button:
                if(d.min!=0||d.max!=1||r.optionCount)return false;break;
            case RowKind::Choice:case RowKind::Cycler:
                if(!r.options||d.step!=1||r.optionCount!=int(d.max-d.min)+1)return false;
                for(int i=0;i<r.optionCount;++i)if(r.options[i].value!=d.min+float(i))return false;
                if(r.kind==RowKind::Choice&&r.optionCount>5)return false;
                break;
            case RowKind::Slider:if(open||r.optionCount)return false;break;
            case RowKind::Stepper:if(r.optionCount)return false;break;
            case RowKind::Swatch:if(!r.format.hueNames||r.optionCount)return false;break;
        }
        if(!IsChoiceKind(r.kind)&&r.options)return false;
        if(r.format.iniScale<=0)return false;
        for(int i=0;i<r.nameCount;++i)if(r.names[i].value<d.min||r.names[i].value>d.max)return false;
        if(r.recommended==r.recommended&&(r.recommended<d.min||r.recommended>d.max))return false;   // NaN: none
    }
    return true;
}
// Rows run page by page (View order) and section by section; each section
// index names a section of its page; every settings page has 1-8 rows and
// every section at least one row.
constexpr int SectionCount(View v){int n=0;for(const auto& s:SectionTable)if(s.page==v)++n;return n;}
constexpr bool OrderOk(){
    for(size_t i=0;i<std::size(RowTable);++i){
        const auto& r=RowTable[i];
        if(!PageTable[size_t(r.page)].settings||r.section>=SectionCount(r.page))return false;
        if(i){const auto& q=RowTable[i-1];if(q.page>r.page||(q.page==r.page&&q.section>r.section))return false;}
    }
    for(const auto& p:PageTable){
        int rows=0;for(const auto& r:RowTable)if(r.page==p.id)++rows;
        if(p.settings!=(rows>0)||rows>8||SectionCount(p.id)>3)return false;
        for(int s=0;s<SectionCount(p.id);++s){int n=0;for(const auto& r:RowTable)if(r.page==p.id&&r.section==s)++n;if(!n)return false;}
    }
    for(size_t i=1;i<std::size(SectionTable);++i)if(SectionTable[i-1].page>SectionTable[i].page)return false;
    return true;
}
// Presets: in range, whole where the setting is whole, each key once and with
// a row, and never the owner's excluded keys.
constexpr bool PresetsOk(){
    for(const auto& p:PresetTable){
        for(int i=0;i<p.count;++i){
            const Key k=p.values[i].key;const float v=p.values[i].value;const auto& d=Definitions[k];
            if(k==StereoRendering||k==DeferGpuWait||k==ElementalHands||k==CompanionAi||k==EyeHeight||k==ArmSpan||k==BodyCalibration)return false;
            if(v<d.min||v>d.max)return false;
            if(d.step==1&&v!=float(int(v)))return false;
            bool row=false;for(const auto& r:RowTable)if(r.key==k&&r.kind!=RowKind::Button)row=true;
            if(!row)return false;
            for(int j=0;j<i;++j)if(p.values[j].key==k)return false;
        }
    }
    return true;
}
static_assert(EveryKeyOnce(),"every Key except NotInMenu appears in exactly one row");
static_assert(TextsFit(),"labels <= 26 (30), help and hints <= 160, warn <= 56, option names <= 14");
static_assert(KindsMatch(),"each row's kind fits its Definitions entry; choices list every ini value in order");
static_assert(OrderOk(),"rows in page and section order; 1-8 rows and 1-3 sections a settings page");
static_assert(PresetsOk(),"presets in range, in ini units, without the excluded keys");
}
}
