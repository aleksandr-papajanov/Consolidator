#include "Host/Editor/EditorPresentation.h"

#include <array>
#include <exception>
#include <stdexcept>
#include <utility>

#include "Features/ChannelStrip/Presentation/ProcessorCatalog.h"
#include "Host/Editor/EditorLayout.h"
#include "Shared/Clients/ParameterEntry.h"
#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::host::editor
{

EditorPresentation::EditorPresentation(EditorServices services)
    : services_(std::move(services)),
      commands_([this](const std::string& selector, std::vector<shared::protocol::ProtocolValue> body)
          { return services_.request(selector, std::move(body)); }),
      targetState_(commands_), stateCommands_(commands_, targetState_),
      analyzerClient_(commands_, targetState_), history_(commands_),
      historyActions_(commands_, nextTransaction_), trackActions_(commands_, registry_, historyActions_),
      tracks_(registry_, trackLevels_, services_.instanceId),
      trackList_(tracks_, [this](const features::tracks::bindings::TrackIntent& intent) { HandleTrackIntent(intent); }),
      instanceButtons_(tracks_, [this](const features::tracks::bindings::TrackIntent& intent)
          { HandleTrackIntent(intent); }, services_.instanceId),
      gesture_([this](const std::string& selector, std::vector<shared::protocol::ProtocolValue> body, Completion completion)
          { commands_.Send(selector, std::move(body), std::move(completion)); },
          [this](bool accepted) { GestureFinished(accepted); }),
      analyzerBindings_(targetState_, gesture_, {
          [this](const std::string& path, std::vector<shared::protocol::StateWriteEntry> values)
              { return BeginGesture(path, std::move(values), true); },
          [this] { SetBusy(busy_); },
          [this](const std::string& path) { ResetValue(path); },
          [this](const std::string& path, bool bypass) { CommitDiscreteValue(path, bypass ? 1 : 0); } }),
      equalizer_(analyzerBindings_, targetState_, tracks_, {
          [this](const features::tracks::bindings::TrackIntent& intent) { HandleTrackIntent(intent); },
          [this](const std::string& path, std::int64_t value) { CommitDiscreteValue(path, value); },
          [this](const std::string& selector) { ProcessBank(selector); },
          [this] { return services_.sampleRate(); } }),
      channelStripBindings_(targetState_, gesture_, {
          [this](const std::string& path, std::int64_t value) { CommitDiscreteValue(path, value); },
          [this](const std::string& path, std::vector<shared::protocol::StateWriteEntry> values)
              { return BeginGesture(path, std::move(values), false); },
          [this] { SetBusy(busy_); },
          [this](const std::string& path) { ResetValue(path); },
          [this](const std::vector<shared::protocol::StateWriteEntry>& values)
          {
              analyzerBindings_.ApplyPreview(values);
              equalizer_.Graph().Refresh();
          },
          [this](const shared::clients::ParameterEntry& entry)
          {
              analyzerBindings_.Apply(entry);
          } }),
      session_(commands_, services_.windowFocused, {
          [this]
          {
              gesture_.Cancel();
              targetState_.Invalidate();
              registry_.Invalidate();
              SetBusy(true);
              status_.setText("Connecting…", juce::dontSendNotification);
          },
          [this] { FetchRegistry(); Observe(); },
          [this]
          {
              gesture_.Close();
              GestureFinished(false);
              targetState_.Invalidate();
              analyzerClient_.Invalidate();
              SetBusy(true);
              status_.setText("Focus this window to edit.", juce::dontSendNotification);
          },
          [this]
          {
              SetBusy(true);
              status_.setText("Unable to activate this editor.", juce::dontSendNotification);
          },
          [this] { UpdateInactiveOverlay(); } })
{
    setLookAndFeel(&lookAndFeel_);
    title_.setText("Consolidator", juce::dontSendNotification);
    for (auto* title : { &title_, &trackTitle_ })
    {
        title->setFont(juce::Font(juce::FontOptions("Arial", shared::ui::kTitleFontHeight, juce::Font::plain)));
        title->setColour(juce::Label::textColourId, shared::ui::kActiveText);
        addAndMakeVisible(title);
    }
    addAndMakeVisible(status_);
    reset_.onClick = [this] { ResetAll(); };
    undo_.onClick = [this]
    {
        SetBusy(true);
        history_.Undo(CommandCompletion());
    };
    redo_.onClick = [this]
    {
        SetBusy(true);
        history_.Redo(CommandCompletion());
    };
    group_.onClick = [this] { GroupSelectedBanks(); };
    ungroup_.onClick = [this] { UngroupFocusedBank(); };
    clear_.onClick = [this] { ClearGroups(); };
    scope_.onClick = [this] { SetGroupScope(!tracks_.GroupScope()); };
    scale_.onClick = [this]
    {
        scale_.setButtonText(juce::String(services_.cycleScale()) + "%");
    };
    trackListToggle_.setTooltip("Hide Track List");
    trackListToggle_.onClick = [this]
    {
        trackListExpanded_ = !trackListExpanded_;
        trackListToggle_.setButtonText(trackListExpanded_ ? "Hide Tracks" : "Show Tracks");
        trackListToggle_.setTooltip(trackListExpanded_ ? "Hide Track List" : "Show Track List");
        services_.resizeEditor();
        repaint();
    };
    for (auto* component : std::array<juce::Component*, 10>{ &reset_, &undo_, &redo_,
            &group_, &ungroup_, &clear_, &scope_, &trackList_, &scale_, &trackListToggle_ })
    {
        addAndMakeVisible(component);
    }
    addAndMakeVisible(channelStrip_);

    addAndMakeVisible(equalizer_);
    addAndMakeVisible(instanceButtons_);
    addAndMakeVisible(inactiveOverlay_);

    setSize(LogicalWidth(), kEditorHeight);
    SetBusy(true);
}

void EditorPresentation::Start()
{
    if (services_.ready)
    {
        session_.Initialize();
    }
    else
    {
        status_.setText("Managed runtime could not be loaded.", juce::dontSendNotification);
    }
}

void EditorPresentation::HandleOutput(const shared::protocol::ProtocolFrame& frame)
{
    try
    {
        RouteOutput(frame);
    }
    catch (const std::exception& exception)
    {
        services_.log(exception.what());
        equalizer_.Graph().Clear();
        session_.Fail();
        gesture_.Cancel();
        targetState_.Invalidate();
        analyzerClient_.Invalidate();
        SetBusy(true);
        status_.setText("Unable to apply the current state. Refocus this window to reconnect.",
            juce::dontSendNotification);
    }
}

EditorPresentation::~EditorPresentation()
{
    closing_ = true;

    gesture_.Close();
    historyActions_.Close();

    session_.Close();
    commands_.Clear();
    setLookAndFeel(nullptr);
}

int EditorPresentation::LogicalWidth() const
{
    return EditorLayout::Width(trackListExpanded_);
}

void EditorPresentation::paint(juce::Graphics& graphics)
{
    graphics.fillAll(shared::ui::kBackground);
    graphics.setColour(shared::ui::kLines);
    graphics.drawVerticalLine(kSidebarWidth - 1, 0.0F, static_cast<float>(kEditorHeight));
    if (trackListExpanded_)
    {
        graphics.drawVerticalLine(LogicalWidth() - kProcessingWidth - 10,
            16.0F, static_cast<float>(kEditorHeight - 16));
    }
}

void EditorPresentation::resized()
{
    const auto bounds = EditorLayout::Calculate(trackListExpanded_,
        juce::GlyphArrangement::getStringWidthInt(trackTitle_.getFont(), trackTitle_.getText()));
    inactiveOverlay_.setBounds(getLocalBounds());
    const std::array<juce::Button*, 5> sidebarActions{
        &trackListToggle_, &undo_, &redo_, &scale_, &scope_ };
    for (std::size_t index = 0; index < sidebarActions.size(); ++index)
    {
        sidebarActions[index]->setBounds(bounds.sidebarActions[index]);
    }
    title_.setBounds(bounds.brandingTitle);
    title_.setVisible(trackListExpanded_);
    trackTitle_.setBounds(bounds.trackTitle);
    instanceButtons_.setBounds(bounds.trackButtons);
    trackList_.setBounds(bounds.trackList);
    trackList_.setVisible(trackListExpanded_);
    const std::array<juce::Button*, 4> actions{ &group_, &ungroup_, &clear_, &reset_ };
    for (std::size_t index = 0; index < actions.size(); ++index)
    {
        actions[index]->setBounds(bounds.trackActions[index]);
        actions[index]->setVisible(trackListExpanded_);
    }
    equalizer_.setBounds(bounds.equalizer);
    channelStrip_.setBounds(bounds.channelStrip);
    status_.setBounds(bounds.status);
}

std::string EditorPresentation::Scope() const { return tracks_.GroupScope() ? "group" : "local"; }

void EditorPresentation::RefreshTrackTitle()
{
    const auto* instance = registry_.Find(tracks_.FocusedInstance());
    trackTitle_.setText(instance != nullptr ? juce::String(instance->label) : juce::String{},
        juce::dontSendNotification);
    instanceButtons_.SetInstance(tracks_.FocusedInstance());
    PositionTrackHeader();
}

void EditorPresentation::PositionTrackHeader()
{
    const auto bounds = EditorLayout::Calculate(trackListExpanded_,
        juce::GlyphArrangement::getStringWidthInt(trackTitle_.getFont(), trackTitle_.getText()));
    trackTitle_.setBounds(bounds.trackTitle);
    instanceButtons_.setBounds(bounds.trackButtons);
}

EditorPresentation::Completion EditorPresentation::CommandCompletion()
{
    const auto generation = targetState_.Generation();
    return [this, generation](bool accepted)
    {
        if (closing_ || !session_.Active() || generation != targetState_.Generation())
        {
            return;
        }
        if (accepted)
        {
            // Accepted values and metadata arrive through the ordered notification stream.
            SetBusy(false);
        }
        else
        {
            Observe();
        }
    };
}

void EditorPresentation::Observe()
{
    if (!session_.Active() || closing_)
    {
        return;
    }
    SetBusy(true);
    equalizer_.Graph().Clear();
    telemetry_.Reset();
    channelStripBindings_.Input().ClearTelemetry();
    channelStrip_.RefreshPath(std::string(channelStripBindings_.Input().OverlayPath()));
    analyzerClient_.Invalidate();
    resized();
    trackList_.RefreshPresentation();
    RefreshTrackTitle();
    targetState_.Observe({ tracks_.FocusedInstance(), tracks_.FocusedBank() });
    FetchBankSnapshots();
}

void EditorPresentation::FetchBankSnapshots()
{
    if (session_.Active() && !closing_)
    {
        analyzerClient_.FetchBanks();
    }
}

void EditorPresentation::FetchRegistry()
{
    if (!registry_.FetchPending() && session_.Active())
    {
        registry_.StartSnapshot(commands_.Request("registry"));
    }
}

void EditorPresentation::SetBusy(bool busy)
{
    busy_ = busy;
    UpdateInactiveOverlay();
    const auto editing = session_.Active() && !busy;
    const auto navigation = editing && !gesture_.Active();
    scale_.setEnabled(navigation);
    trackListToggle_.setEnabled(navigation);
    equalizer_.SetEditingState(navigation, editing && (!gesture_.Active() ||
        (gesture_.Editing() && analyzerGesture_ && gesture_.Path().starts_with(equalizer_.ContextPath() + "."))),
        registry_.Ready());

    channelStripBindings_.Parameters().SetEditing(editing);
    channelStrip_.SetEditing(editing);
    reset_.setEnabled(navigation);
    undo_.setEnabled(navigation && history_.CanUndo());
    redo_.setEnabled(navigation && history_.CanRedo());
    trackList_.setEnabled(navigation && registry_.Ready());
    instanceButtons_.setEnabled(navigation && registry_.Ready() && registry_.Find(tracks_.FocusedInstance()) != nullptr);
    instanceButtons_.SetInstance(tracks_.FocusedInstance());
    const auto actions = tracks_.Actions();
    group_.setEnabled(navigation && actions.group);
    const auto bank = registry_.FindBank(tracks_.FocusedInstance(), tracks_.FocusedBank());
    const auto groupColor = bank != nullptr && bank->group ? shared::ui::GroupColor(*bank->group) : shared::ui::kLines;

    channelStripBindings_.Parameters().SetScope(tracks_.GroupScope());
    channelStrip_.SetScopeColor(groupColor);
    ungroup_.setEnabled(navigation && actions.ungroup);
    scope_.setEnabled(navigation && actions.scope);
    clear_.setEnabled(navigation && actions.clear);
}

void EditorPresentation::UpdateInactiveOverlay()
{
    inactiveOverlay_.setVisible(!session_.Focused() || !session_.Active());
}

void EditorPresentation::ApplySnapshot(const shared::protocol::ProtocolFrame& frame)
{
    if (!session_.Active() || !targetState_.HandleSnapshot(frame))
    {
        return;
    }

    channelStrip_.Clear();
    channelStripBindings_.ApplySnapshot();
    equalizer_.ApplySnapshot();

    channelStrip_.Rebuild(channelStripBindings_);
    resized();
    SetBusy(false);
    status_.setText("", juce::dontSendNotification);
}

void EditorPresentation::ApplyChangedValue(const shared::protocol::ProtocolFrame& frame)
{
    if (!session_.Active())
    {
        return;
    }
    const auto entry = targetState_.HandleChanged(frame);
    if (!entry)
    {
        return;
    }
    const auto update = channelStripBindings_.ApplyChanged(*entry);
    if (update.parameterChanged)
    {
        channelStrip_.RefreshPath(entry->path);
    }
    if (update.runtimeOverlayChanged)
    {
        channelStrip_.RefreshPath(std::string(channelStripBindings_.Input().OverlayPath()));
    }
    equalizer_.Graph().Refresh();
    equalizer_.RefreshAcceptedState();
    if (entry->path.starts_with("equalizer."))
    {
        FetchBankSnapshots();
    }
}

void EditorPresentation::ApplyTelemetry(const shared::protocol::ProtocolFrame& frame)
{
    if (!session_.Active() || busy_ || services_.sampleRate() <= 0)
    {
        return;
    }
    const auto telemetry = telemetry_.Handle(frame, tracks_.FocusedInstance());
    if (!telemetry)
    {
        return;
    }
    channelStripBindings_.Input().ApplyTelemetry(telemetry->slow, telemetry->fast);
    channelStrip_.RefreshPath(std::string(channelStripBindings_.Input().OverlayPath()));
}

void EditorPresentation::RouteOutput(const shared::protocol::ProtocolFrame& frame)
{
    if (frame.selector == "audio_configuration_changed")
    {
        equalizer_.Graph().SetSampleRate(shared::protocol::Number(frame.values.at(0)));

        telemetry_.Reset();
        channelStripBindings_.Input().ClearTelemetry();
        channelStrip_.RefreshPath(std::string(channelStripBindings_.Input().OverlayPath()));
        return;
    }
    if (frame.selector == "state_restored")
    {
        gesture_.Cancel();
        Observe();
        return;
    }
    // Non-command notifications keep their version-1 envelope. FFT owns its
    // version-2 schema in AnalyzerClient; it must not enter command validation.
    if (frame.selector != "fft" &&
        (frame.values.empty() || shared::protocol::Integer(frame.values[0]) != 1))
    {
        throw std::invalid_argument("Unsupported Managed notification frame.");
    }
    if (const auto result = commands_.Handle(frame))
    {
        if (!result->accepted)
        {
            services_.log(("Command rejected: instance=" + std::to_string(services_.instanceId) +
                " request=" + std::to_string(result->request) + " reason=" + result->message));
            if (!result->matched)
            {
                gesture_.Cancel();
            }
        }
        if (frame.selector == "error")
        {
            registry_.CancelSnapshot(result->request);
            if (targetState_.IsSnapshotRequest(result->request) || !result->matched)
            {
                session_.Fail();
                SetBusy(true);
            }
            status_.setText(juce::String(result->message), juce::dontSendNotification);
        }
        else if (!result->accepted)
        {
            status_.setText("The change was rejected.", juce::dontSendNotification);
        }
    }
    else if (frame.selector == "target_state_snapshot")
    {
        ApplySnapshot(frame);
    }
    else if (frame.selector == "state_changed")
    {
        ApplyChangedValue(frame);
    }
    else if (frame.selector == "bank_state_snapshots")
    {
        if (session_.Active())
        {
            if (auto banks = analyzerClient_.HandleBanks(frame))
            {
                equalizer_.Graph().ApplyBankSnapshots(std::move(*banks));
            }
        }
    }
    else if (frame.selector == "fft")
    {
        if (session_.Active() && !busy_)
        {
            if (auto spectrum = analyzerClient_.HandleSpectrum(frame))
            {
                equalizer_.Graph().ApplySpectrum(std::move(*spectrum));
            }
        }
    }
    else if (frame.selector == "dsp_telemetry")
    {
        ApplyTelemetry(frame);
    }
    else if (history_.Handle(frame))
    {
        SetBusy(busy_);
    }
    else if (trackLevels_.Handle(frame))
    {
        trackList_.repaint();
    }
    else
    {
        const auto update = registry_.Handle(frame);
        if (update == shared::clients::RegistryUpdate::Resynchronize)
        {
            FetchRegistry();
        }
        else if (update == shared::clients::RegistryUpdate::Changed)
        {
            RegistryChanged();
        }
        else if (update == shared::clients::RegistryUpdate::PresentationChanged)
        {
            tracks_.ConfirmFeedback();
            trackList_.RefreshPresentation();
            equalizer_.RefreshAcceptedState();
            instanceButtons_.RefreshPresentation();
        }
    }
}

void EditorPresentation::RegistryChanged()
{
    tracks_.Refresh();
    trackList_.Refresh();
    if (session_.Active() && registry_.Find(tracks_.FocusedInstance()) == nullptr)
    {
        gesture_.Cancel();
        tracks_.ClearSelection();
        tracks_.SetFocus(services_.instanceId, 0, false);
        SetGroupScope(false);
        Observe();
    }
    const auto bank = registry_.FindBank(tracks_.FocusedInstance(), tracks_.FocusedBank());
    if (tracks_.GroupScope() && (bank == nullptr || !bank->group))
    {
        gesture_.Cancel();
        SetGroupScope(false);
    }
    RefreshTrackTitle();
    SetBusy(busy_);
}

void EditorPresentation::SelectTarget(std::uint64_t instance, int bank)
{
    if (busy_ || gesture_.Active() || !session_.Active() || registry_.FindBank(instance, bank) == nullptr)
    {
        return;
    }
    const auto sameBank = tracks_.FocusedInstance() == instance && tracks_.FocusedBank() == bank;
    if (sameBank)
    {
        SetBusy(false);
        return;
    }
    tracks_.SetFocus(instance, bank, registry_.FindBank(instance, bank)->group.has_value());
    SetGroupScope(registry_.FindBank(instance, bank)->group.has_value());
    Observe();
}

void EditorPresentation::SetGroupScope(bool groupScope)
{
    tracks_.SetFocus(tracks_.FocusedInstance(), tracks_.FocusedBank(), groupScope);

    scope_.setButtonText(groupScope ? "Group" : "Local");
    const auto bank = registry_.FindBank(tracks_.FocusedInstance(), tracks_.FocusedBank());
    scope_.setToggleState(groupScope, juce::dontSendNotification);
    scope_.setColour(juce::TextButton::buttonOnColourId,
        bank != nullptr && bank->group ? shared::ui::GroupColor(*bank->group) : shared::ui::kActive);
    channelStripBindings_.Parameters().SetScope(groupScope);
    channelStrip_.SetScopeColor(bank != nullptr && bank->group ? shared::ui::GroupColor(*bank->group) : shared::ui::kLines);
    trackList_.RefreshPresentation();
    equalizer_.RefreshAcceptedState();
    instanceButtons_.RefreshPresentation();
}

bool EditorPresentation::BeginGesture(const std::string& path, std::vector<shared::protocol::StateWriteEntry> values, bool analyzerGesture)
{
    if (busy_ || gesture_.Active() || !session_.Active())
    {
        return false;
    }
    if (channelStripBindings_.Input().IsAutomatedValue(path))
    {
        return false;
    }
    analyzerBindings_.ApplyPreview(values);
    equalizer_.Graph().Refresh();
    analyzerGesture_ = analyzerGesture;
    gesture_.Begin(nextTransaction_++, path, std::move(values), Scope());
    SetBusy(false);
    return true;
}

void EditorPresentation::GestureFinished(bool accepted)
{
    equalizer_.Graph().FinishPreview();

    channelStripBindings_.Parameters().FinishPreview();
    channelStrip_.FinishPreview(accepted);
    SetBusy(busy_);
    if (!accepted && !closing_)
    {
        Observe();
    }
}

void EditorPresentation::CommitDiscreteValue(const std::string& path, std::int64_t value)
{
    if (busy_ || gesture_.Active() || !session_.Active())
    {
        return;
    }
    const auto generation = targetState_.Generation();
    const auto scope = tracks_.GroupScope();
    const auto completion = [this, generation, scope](bool accepted)
    {
        if (!accepted && !busy_ && session_.Active() && generation == targetState_.Generation() && scope == tracks_.GroupScope())
        {
            Observe();
        }
    };
    if (features::channelstrip::presentation::UsesDiscreteHistory(path))
    {
        historyActions_.Run([this, path, value, generation, scope](const std::string& transaction, Completion done)
        {
            if (busy_ || !session_.Active() || !targetState_.Ready() || generation != targetState_.Generation() ||
                scope != tracks_.GroupScope())
            {
                services_.log("Canceled discrete edit after target/scope transition: " + path);
                done(false);
                return;
            }
            stateCommands_.WriteDiscrete(path, value, scope, transaction, std::move(done));
        }, completion);
    }
    else
    {
        stateCommands_.WriteDiscrete(path, value, tracks_.GroupScope(), "0", completion);
    }
}

void EditorPresentation::ResetValue(const std::string& path)
{
    if (busy_ || gesture_.Active() || !session_.Active() ||
        channelStripBindings_.Input().IsAutomatedValue(path))
    {
        return;
    }
    SetBusy(true);
    stateCommands_.ResetValue(path, tracks_.GroupScope(), CommandCompletion());
}

void EditorPresentation::ResetAll()
{
    SetBusy(true);
    stateCommands_.ResetValue("dsp", tracks_.GroupScope(), CommandCompletion());
}

void EditorPresentation::HandleTrackIntent(const features::tracks::bindings::TrackIntent& intent)
{
    trackList_.repaint();
    equalizer_.RefreshAcceptedState();
    if (busy_ || gesture_.Active() || !session_.Active())
    {
        return;
    }
    if (intent.kind == features::tracks::bindings::TrackIntentKind::SelectionChanged)
    {
        SetBusy(false);
        return;
    }
    if (intent.kind == features::tracks::bindings::TrackIntentKind::SelectBank || intent.kind == features::tracks::bindings::TrackIntentKind::SelectProcessor)
    {
        SelectTarget(intent.instance, intent.bank);
        return;
    }
    SetBusy(true);
    trackActions_.Execute(intent, targetState_.Target(), tracks_.GroupScope(), [this, completion = CommandCompletion()](bool accepted)
    {
        if (!accepted)
        {
            tracks_.Feedback().Clear();
            trackList_.repaint();
        }
        completion(accepted);
    });
}

void EditorPresentation::GroupSelectedBanks()
{
    if (!registry_.NextGroup() || tracks_.SelectedBanks().size() < 2)
    {
        return;
    }
    const features::tracks::controllers::TrackController::Banks banks(
        tracks_.SelectedBanks().begin(), tracks_.SelectedBanks().end());
    SetBusy(true);
    trackActions_.Group(banks, [this, banks](bool accepted)
    {
        if (accepted)
        {
            tracks_.ClearSelection();
            tracks_.SetFocus(banks.front().first, banks.front().second, true);
            SetGroupScope(true);
        }
        Observe();
    });
}

void EditorPresentation::UngroupFocusedBank()
{
    SetBusy(true);
    trackActions_.Ungroup(targetState_.Target(), [this, completion = CommandCompletion()](bool accepted)
    {
        if (accepted)
        {
            tracks_.ClearSelection();
            SetGroupScope(false);
        }
        completion(accepted);
    });
}

void EditorPresentation::ClearGroups()
{
    const juce::Component::SafePointer<EditorPresentation> editor(this);
    juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon, "Clear groups",
        "Clear all user groups? The automatic group A will remain.", "Clear", "Cancel", this,
        juce::ModalCallbackFunction::create([editor](int result)
        {
            if (result != 0 && editor != nullptr && !editor->busy_ && editor->session_.Active() && !editor->gesture_.Active())
            {
                editor->SetBusy(true);
                editor->trackActions_.Clear(editor->CommandCompletion());
            }
        }));
}

void EditorPresentation::ProcessBank(const std::string& selector)
{
    // Commit/Refine handlers advance history only after successful optimization.
    SetBusy(true);
    trackActions_.Process(selector, targetState_.Target(), tracks_.GroupScope(), CommandCompletion());
}
} // namespace consolidator::vst3::host::editor
