// Copyright (c) - SurgeTechnologies - All rights reserved
#include "UILayoutBinaryFormat.hpp"
#include "Surge/Asset/Serializer/BinaryHelpers.hpp"
#include "Surge/Graphics/UISystem/UILayout.hpp"
#include "Surge/Utility/Filesystem.hpp"

namespace Surge::UILayoutBinary
{
    static constexpr Uint MAGIC = 0x55444952; // RIDU
    static constexpr Uint MAX_DEPTH = 64;     // Deeper trees are treated as corrupted data

    struct Header
    {
        Uint Magic;
        Uint Version;
    };

    // Release builds of the Windows toolchain (no iterator debugging, so the std containers have their shipping size): a desc struct that
    // changes size breaks the build here. Update Write/Read below, bump VERSION, then update the size
#if defined(SURGE_PLATFORM_WINDOWS) && defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL == 0
    static_assert(sizeof(UI::WidgetDesc) == 856, "UI::WidgetDesc changed: update UILayoutBinary Write/Read, bump VERSION, then this size");
    static_assert(sizeof(UI::ImagePartDesc) == 80, "UI::ImagePartDesc changed: update UILayoutBinary Write/Read, bump VERSION, then this size");
    static_assert(sizeof(UI::CanvasScaler) == 12, "UI::CanvasScaler changed: update UILayoutBinary Write/Read, bump VERSION, then this size");
#endif

    template <typename E>
    static void WriteEnum(Vector<Byte>& buffer, E value)
    {
        WriteData(buffer, static_cast<int32_t>(value));
    }

    template <typename E>
    static E ReadEnum(BinaryReader& reader)
    {
        int32_t value = 0;
        reader.Read(value);
        return static_cast<E>(value);
    }

    static void WriteBool(Vector<Byte>& buffer, bool value)
    {
        WriteData(buffer, static_cast<Byte>(value ? 1 : 0));
    }

    static bool ReadBool(BinaryReader& reader)
    {
        Byte value = 0;
        reader.Read(value);
        return value != 0;
    }

    static void WriteInt(Vector<Byte>& buffer, int value)
    {
        WriteData(buffer, static_cast<int32_t>(value));
    }

    static int ReadInt(BinaryReader& reader)
    {
        int32_t value = 0;
        reader.Read(value);
        return value;
    }

    static void WriteID(Vector<Byte>& buffer, const UUID& id)
    {
        WriteData(buffer, id.Get());
    }

    static UUID ReadID(BinaryReader& reader)
    {
        uint64_t value = 0;
        reader.Read(value);
        return UUID(value);
    }

    static void WriteImagePart(Vector<Byte>& buffer, const UI::ImagePartDesc& part)
    {
        WriteID(buffer, part.Texture);
        WriteData(buffer, part.Color);
        WriteEnum(buffer, part.Mode);
        WriteData(buffer, part.SliceBorder);
        WriteData(buffer, part.SliceScale);
        WriteBool(buffer, part.FillCenter);
        WriteData(buffer, part.CornerRadius);
        WriteData(buffer, part.BorderWidth);
        WriteData(buffer, part.BorderColor);
    }

    static void ReadImagePart(BinaryReader& reader, UI::ImagePartDesc& part)
    {
        part.Texture = ReadID(reader);
        reader.Read(part.Color);
        part.Mode = ReadEnum<UI::ImageMode>(reader);
        reader.Read(part.SliceBorder);
        reader.Read(part.SliceScale);
        part.FillCenter = ReadBool(reader);
        reader.Read(part.CornerRadius);
        reader.Read(part.BorderWidth);
        reader.Read(part.BorderColor);
    }

    static void WriteWidget(Vector<Byte>& buffer, const UI::WidgetDesc& w)
    {
        WriteID(buffer, w.ID);
        WriteStr(buffer, w.Name);
        WriteEnum(buffer, w.Type);
        WriteBool(buffer, w.Visible);

        WriteData(buffer, w.AnchorMin);
        WriteData(buffer, w.AnchorMax);
        WriteData(buffer, w.Pivot);
        WriteData(buffer, w.Offset);
        WriteData(buffer, w.Size);
        WriteData(buffer, w.Color);

        WriteImagePart(buffer, w.Background);
        WriteImagePart(buffer, w.Fill);
        WriteImagePart(buffer, w.Handle);

        WriteData(buffer, w.FillAmount);
        WriteEnum(buffer, w.Origin);
        WriteBool(buffer, w.PreserveAspect);

        WriteStr(buffer, w.Text);
        WriteID(buffer, w.Font);
        WriteData(buffer, w.FontSize);
        WriteEnum(buffer, w.HAlign);
        WriteEnum(buffer, w.VAlign);
        WriteEnum(buffer, w.Overflow);
        WriteData(buffer, w.TextColor);
        WriteData(buffer, w.TextPadding);

        WriteData(buffer, w.NormalColor);
        WriteData(buffer, w.HoverColor);
        WriteData(buffer, w.PressedColor);
        WriteData(buffer, w.HighlightColor);

        WriteData(buffer, w.MinValue);
        WriteData(buffer, w.MaxValue);
        WriteData(buffer, w.Value);
        WriteBool(buffer, w.WholeNumbers);
        WriteData(buffer, w.FillPadding);

        WriteBool(buffer, w.IsOn);
        WriteStr(buffer, w.Group);
        WriteBool(buffer, w.AllowSwitchOff);
        WriteEnum(buffer, w.ToggleType);

        WriteData(buffer, w.HandleSize);
        WriteData(buffer, w.TrackThickness);
        WriteEnum(buffer, w.StickMode);
        WriteData(buffer, w.BaseSize);
        WriteData(buffer, w.DeadZone);
        WriteData(buffer, w.HandleRange);
        WriteBool(buffer, w.HideWhenIdle);

        WriteBool(buffer, w.ScrollHorizontal);
        WriteBool(buffer, w.ScrollVertical);
        WriteData(buffer, w.DecelerationRate);
        WriteBool(buffer, w.ShowScrollbars);
        WriteData(buffer, w.ScrollbarColor);

        WriteEnum(buffer, w.Direction);
        WriteData(buffer, w.Padding);
        WriteData(buffer, w.Spacing);
        WriteData(buffer, w.CellSize);
        WriteInt(buffer, w.Columns);
        WriteData(buffer, w.ChildAlignment);
        WriteBool(buffer, w.ControlChildWidth);
        WriteBool(buffer, w.ControlChildHeight);
        WriteBool(buffer, w.FitContent);

        WriteData(buffer, w.ItemSize);
        WriteInt(buffer, w.PreviewItemCount);

        WriteStr(buffer, w.Placeholder);
        WriteEnum(buffer, w.ContentType);
        WriteBool(buffer, w.MultiLine);
        WriteInt(buffer, w.MaxLength);
        WriteData(buffer, w.PlaceholderColor);
        WriteData(buffer, w.CaretColor);
        WriteData(buffer, w.SelectionColor);

        WriteData(buffer, static_cast<Uint>(w.Options.size()));
        for(const String& option : w.Options)
            WriteStr(buffer, option);
        WriteInt(buffer, w.SelectedIndex);
        WriteInt(buffer, w.MaxVisibleItems);

        WriteData(buffer, w.BackdropColor);
        WriteBool(buffer, w.CloseOnBackdrop);
        WriteBool(buffer, w.CloseOnBack);

        WriteInt(buffer, w.ActiveTab);
        WriteBool(buffer, w.TabsAtBottom);
        WriteData(buffer, w.TabBarHeight);

        WriteInt(buffer, w.DotCount);
        WriteData(buffer, w.DotSize);
        WriteData(buffer, w.Speed);

        WriteInt(buffer, w.SafeEdges);

        WriteData(buffer, static_cast<Uint>(w.Children.size()));
        for(const UI::WidgetDesc& child : w.Children)
            WriteWidget(buffer, child);
    }

    static void ReadWidget(BinaryReader& reader, UI::WidgetDesc& w, Uint depth)
    {
        if(depth > MAX_DEPTH)
        {
            reader.Failed = true;
            return;
        }

        w.ID = ReadID(reader);
        w.Name = reader.ReadStr();
        w.Type = ReadEnum<UI::WidgetType>(reader);
        w.Visible = ReadBool(reader);

        reader.Read(w.AnchorMin);
        reader.Read(w.AnchorMax);
        reader.Read(w.Pivot);
        reader.Read(w.Offset);
        reader.Read(w.Size);
        reader.Read(w.Color);

        ReadImagePart(reader, w.Background);
        ReadImagePart(reader, w.Fill);
        ReadImagePart(reader, w.Handle);

        reader.Read(w.FillAmount);
        w.Origin = ReadEnum<UI::FillOrigin>(reader);
        w.PreserveAspect = ReadBool(reader);

        w.Text = reader.ReadStr();
        w.Font = ReadID(reader);
        reader.Read(w.FontSize);
        w.HAlign = ReadEnum<TextAlignment>(reader);
        w.VAlign = ReadEnum<TextVerticalAlignment>(reader);
        w.Overflow = ReadEnum<UI::TextOverflow>(reader);
        reader.Read(w.TextColor);
        reader.Read(w.TextPadding);

        reader.Read(w.NormalColor);
        reader.Read(w.HoverColor);
        reader.Read(w.PressedColor);
        reader.Read(w.HighlightColor);

        reader.Read(w.MinValue);
        reader.Read(w.MaxValue);
        reader.Read(w.Value);
        w.WholeNumbers = ReadBool(reader);
        reader.Read(w.FillPadding);

        w.IsOn = ReadBool(reader);
        w.Group = reader.ReadStr();
        w.AllowSwitchOff = ReadBool(reader);
        w.ToggleType = ReadEnum<UI::ToggleStyle>(reader);

        reader.Read(w.HandleSize);
        reader.Read(w.TrackThickness);
        w.StickMode = ReadEnum<UI::JoystickMode>(reader);
        reader.Read(w.BaseSize);
        reader.Read(w.DeadZone);
        reader.Read(w.HandleRange);
        w.HideWhenIdle = ReadBool(reader);

        w.ScrollHorizontal = ReadBool(reader);
        w.ScrollVertical = ReadBool(reader);
        reader.Read(w.DecelerationRate);
        w.ShowScrollbars = ReadBool(reader);
        reader.Read(w.ScrollbarColor);

        w.Direction = ReadEnum<UI::LayoutDirection>(reader);
        reader.Read(w.Padding);
        reader.Read(w.Spacing);
        reader.Read(w.CellSize);
        w.Columns = ReadInt(reader);
        reader.Read(w.ChildAlignment);
        w.ControlChildWidth = ReadBool(reader);
        w.ControlChildHeight = ReadBool(reader);
        w.FitContent = ReadBool(reader);

        reader.Read(w.ItemSize);
        w.PreviewItemCount = ReadInt(reader);

        w.Placeholder = reader.ReadStr();
        w.ContentType = ReadEnum<UI::InputContentType>(reader);
        w.MultiLine = ReadBool(reader);
        w.MaxLength = ReadInt(reader);
        reader.Read(w.PlaceholderColor);
        reader.Read(w.CaretColor);
        reader.Read(w.SelectionColor);

        Uint optionCount = 0;
        reader.Read(optionCount);
        if(!reader.Fits(optionCount, sizeof(Uint)))
        {
            reader.Failed = true;
            return;
        }
        w.Options.resize(optionCount);
        for(String& option : w.Options)
            option = reader.ReadStr();
        w.SelectedIndex = ReadInt(reader);
        w.MaxVisibleItems = ReadInt(reader);

        reader.Read(w.BackdropColor);
        w.CloseOnBackdrop = ReadBool(reader);
        w.CloseOnBack = ReadBool(reader);

        w.ActiveTab = ReadInt(reader);
        w.TabsAtBottom = ReadBool(reader);
        reader.Read(w.TabBarHeight);

        w.DotCount = ReadInt(reader);
        reader.Read(w.DotSize);
        reader.Read(w.Speed);

        w.SafeEdges = ReadInt(reader);

        Uint childCount = 0;
        reader.Read(childCount);
        if(!reader.Fits(childCount, 1))
        {
            reader.Failed = true;
            return;
        }
        w.Children.resize(childCount);
        for(Uint i = 0; i < childCount && !reader.Failed; i++)
            ReadWidget(reader, w.Children[i], depth + 1);
    }

    bool Write(const String& path, const AssetStamp& stamp, const UILayout& layout)
    {
        Vector<Byte> buffer;
        buffer.reserve(4096);
        WriteData(buffer, stamp);

        const Header header = { MAGIC, VERSION };
        WriteData(buffer, header);
        WriteData(buffer, layout.Scaler.ReferenceResolution);
        WriteData(buffer, layout.Scaler.MatchWidthOrHeight);
        WriteWidget(buffer, layout.Root);

        if(!Filesystem::WriteBinaryFile(path, buffer.data(), buffer.size()))
        {
            Log<Severity::Error>("[UILayoutBinary] Failed to write {}", path);
            return false;
        }
        Log<Severity::Trace>("[UILayoutBinary] Cooked {}", path);
        return true;
    }

    bool Read(const String& path, AssetStamp& outStamp, UILayout& outLayout)
    {
        Vector<Byte> fileData;
        if(!Filesystem::ReadBinaryFile(path, fileData))
        {
            Log<Severity::Error>("[UILayoutBinary] Failed to read {}", path);
            return false;
        }

        BinaryReader reader(fileData.data(), fileData.size());
        reader.Read(outStamp);

        Header header = {};
        reader.Read(header);
        if(reader.Failed || header.Magic != MAGIC)
        {
            Log<Severity::Error>("[UILayoutBinary] {} is not a cooked UI layout!", path);
            return false;
        }
        if(header.Version != VERSION)
        {
            Log<Severity::Error>("[UILayoutBinary] {} has version {}, expected {}. Re-cook it in the Editor (or export the game again)", path, header.Version, VERSION);
            return false;
        }

        UI::CanvasScaler scaler;
        reader.Read(scaler.ReferenceResolution);
        reader.Read(scaler.MatchWidthOrHeight);

        UI::WidgetDesc root;
        ReadWidget(reader, root, 0);
        if(reader.Failed)
        {
            Log<Severity::Error>("[UILayoutBinary] {} is truncated or corrupted!", path);
            return false;
        }

        outLayout.Scaler = scaler;
        outLayout.Root = std::move(root);
        return true;
    }
}
