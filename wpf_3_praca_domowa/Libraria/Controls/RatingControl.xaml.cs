using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Libraria.Controls
{
    /// <summary>
    /// Interaction logic for RatingControl.xaml
    /// </summary>
    public partial class RatingControl : UserControl
    {
        public static readonly DependencyProperty ValueProperty =
            DependencyProperty.Register(
                nameof(Value),
                typeof(int),
                typeof(RatingControl),
                new FrameworkPropertyMetadata(
                    0,
                    FrameworkPropertyMetadataOptions.BindsTwoWayByDefault,
                    OnRatingPropertyChanged,
                    CoerceValue));

        public static readonly DependencyProperty IsReadOnlyProperty =
            DependencyProperty.Register(
                nameof(IsReadOnly),
                typeof(bool),
                typeof(RatingControl),
                new PropertyMetadata(false, OnRatingPropertyChanged));

        public static readonly DependencyProperty StarSizeProperty =
            DependencyProperty.Register(
                nameof(StarSize),
                typeof(double),
                typeof(RatingControl),
                new PropertyMetadata(22.0));

        public RatingControl()
        {
            SetRatingCommand = new SetRatingValueCommand(this);
            InitializeComponent();
        }

        public int Value
        {
            get => (int)GetValue(ValueProperty);
            set => SetValue(ValueProperty, value);
        }

        public bool IsReadOnly
        {
            get => (bool)GetValue(IsReadOnlyProperty);
            set => SetValue(IsReadOnlyProperty, value);
        }

        public double StarSize
        {
            get => (double)GetValue(StarSizeProperty);
            set => SetValue(StarSizeProperty, value);
        }

        public ICommand SetRatingCommand { get; }

        private static object CoerceValue(DependencyObject d, object baseValue)
        {
            return Math.Clamp((int)baseValue, 0, 5);
        }

        private static void OnRatingPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
        }

        private sealed class SetRatingValueCommand : ICommand
        {
            private readonly RatingControl _control;

            public SetRatingValueCommand(RatingControl control)
            {
                _control = control;
            }

            public event EventHandler? CanExecuteChanged
            {
                add => CommandManager.RequerySuggested += value;
                remove => CommandManager.RequerySuggested -= value;
            }

            public bool CanExecute(object? parameter)
            {
                return true;
            }

            public void Execute(object? parameter)
            {
                if (_control.IsReadOnly)
                {
                    return;
                }

                if (parameter is string text && int.TryParse(text, out var value))
                {
                    _control.Value = value;
                }
            }
        }
    }
}
