using System.Windows;
using System.Windows.Controls;

namespace Libraria.Controls
{
    /// <summary>
    /// Interaction logic for StatisticsBarControl.xaml
    /// </summary>
    public partial class StatisticsBarControl : UserControl
    {
        public static readonly DependencyProperty TotalBooksProperty =
            DependencyProperty.Register(
                nameof(TotalBooks),
                typeof(int),
                typeof(StatisticsBarControl),
                new PropertyMetadata(0));

        public static readonly DependencyProperty ReadBooksProperty =
            DependencyProperty.Register(
                nameof(ReadBooks),
                typeof(int),
                typeof(StatisticsBarControl),
                new PropertyMetadata(0));

        public static readonly DependencyProperty InProgressBooksProperty =
            DependencyProperty.Register(
                nameof(InProgressBooks),
                typeof(int),
                typeof(StatisticsBarControl),
                new PropertyMetadata(0));

        public static readonly DependencyProperty AverageRatingProperty =
            DependencyProperty.Register(
                nameof(AverageRating),
                typeof(string),
                typeof(StatisticsBarControl),
                new PropertyMetadata("—"));

        public static readonly DependencyProperty TopGenreProperty =
            DependencyProperty.Register(
                nameof(TopGenre),
                typeof(string),
                typeof(StatisticsBarControl),
                new PropertyMetadata("—"));

        public static readonly DependencyProperty StatusMessageProperty =
            DependencyProperty.Register(
                nameof(StatusMessage),
                typeof(string),
                typeof(StatisticsBarControl),
                new PropertyMetadata(string.Empty));

        public StatisticsBarControl()
        {
            InitializeComponent();
        }

        public int TotalBooks
        {
            get => (int)GetValue(TotalBooksProperty);
            set => SetValue(TotalBooksProperty, value);
        }

        public int ReadBooks
        {
            get => (int)GetValue(ReadBooksProperty);
            set => SetValue(ReadBooksProperty, value);
        }

        public int InProgressBooks
        {
            get => (int)GetValue(InProgressBooksProperty);
            set => SetValue(InProgressBooksProperty, value);
        }

        public string AverageRating
        {
            get => (string)GetValue(AverageRatingProperty);
            set => SetValue(AverageRatingProperty, value);
        }

        public string TopGenre
        {
            get => (string)GetValue(TopGenreProperty);
            set => SetValue(TopGenreProperty, value);
        }

        public string StatusMessage
        {
            get => (string)GetValue(StatusMessageProperty);
            set => SetValue(StatusMessageProperty, value);
        }
    }
}
