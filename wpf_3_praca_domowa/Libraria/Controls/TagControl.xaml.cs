using System.Collections;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace Libraria.Controls
{
    /// <summary>
    /// Interaction logic for TagControl.xaml
    /// </summary>
    public partial class TagControl : UserControl
    {
        public static readonly DependencyProperty TagsProperty =
            DependencyProperty.Register(
                nameof(Tags),
                typeof(IEnumerable),
                typeof(TagControl),
                new PropertyMetadata(null));

        public static readonly DependencyProperty IsEditableProperty =
            DependencyProperty.Register(
                nameof(IsEditable),
                typeof(bool),
                typeof(TagControl),
                new PropertyMetadata(false));

        public static readonly DependencyProperty RemoveTagCommandProperty =
            DependencyProperty.Register(
                nameof(RemoveTagCommand),
                typeof(ICommand),
                typeof(TagControl),
                new PropertyMetadata(null));

        public static readonly DependencyProperty ChipBackgroundProperty =
            DependencyProperty.Register(
                nameof(ChipBackground),
                typeof(Brush),
                typeof(TagControl),
                new PropertyMetadata(Brushes.SteelBlue));

        public static readonly DependencyProperty ChipForegroundProperty =
            DependencyProperty.Register(
                nameof(ChipForeground),
                typeof(Brush),
                typeof(TagControl),
                new PropertyMetadata(Brushes.White));

        public TagControl()
        {
            InitializeComponent();
        }

        public IEnumerable? Tags
        {
            get => (IEnumerable?)GetValue(TagsProperty);
            set => SetValue(TagsProperty, value);
        }

        public bool IsEditable
        {
            get => (bool)GetValue(IsEditableProperty);
            set => SetValue(IsEditableProperty, value);
        }

        public ICommand? RemoveTagCommand
        {
            get => (ICommand?)GetValue(RemoveTagCommandProperty);
            set => SetValue(RemoveTagCommandProperty, value);
        }

        public Brush ChipBackground
        {
            get => (Brush)GetValue(ChipBackgroundProperty);
            set => SetValue(ChipBackgroundProperty, value);
        }

        public Brush ChipForeground
        {
            get => (Brush)GetValue(ChipForegroundProperty);
            set => SetValue(ChipForegroundProperty, value);
        }
    }
}
