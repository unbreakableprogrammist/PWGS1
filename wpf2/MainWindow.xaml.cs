using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;

namespace WpfApp // Pamiętaj, aby nazwa namespace zgadzała się z Twoim projektem
{
    public partial class MainWindow : Window
    {
        // Zmienne pomocnicze do logiki przeciągania elementów myszką
        private bool isDragging = false;
        private Shape draggedShape = null;
        private Point clickPosition;
        private Shape selectedShape = null;

        public MainWindow()
        {
            InitializeComponent();
        }

        // RYSOWANIE PROSTYCH KSZTAŁTÓW
        private void AddShape_Click(object sender, RoutedEventArgs e)
        {
            Rectangle rect = new Rectangle
            {
                Fill = Brushes.SteelBlue,
                Stroke = Brushes.Transparent,
                StrokeThickness = 3,
                ContextMenu = (ContextMenu)this.Resources["ShapeContextMenu"] // Podpięcie menu
            };

            // DATA BINDING w kodzie: Łączymy szerokość i wysokość kwadratu z suwakiem z XAML-a
            Binding sizeBinding = new Binding("Value") { Source = SizeSlider };
            rect.SetBinding(Rectangle.WidthProperty, sizeBinding);
            rect.SetBinding(Rectangle.HeightProperty, sizeBinding);

            // Pozycja początkowa na Canvasie
            Canvas.SetLeft(rect, 100);
            Canvas.SetTop(rect, 100);

            MainCanvas.Children.Add(rect);
        }

        // OBSŁUGA ZDARZEŃ MYSZKI: Zaznaczanie i start przeciągania (MouseLeftButtonDown)
        private void Canvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (e.OriginalSource is Shape shape)
            {
                // Odznaczamy stary kształt i zaznaczamy nowy pomarańczową ramką
                if (selectedShape != null) selectedShape.Stroke = Brushes.Transparent;
                selectedShape = shape;
                selectedShape.Stroke = Brushes.Orange;

                // Aktywujemy tryb przeciągania
                isDragging = true;
                draggedShape = shape;
                clickPosition = e.GetPosition(shape);
                shape.CaptureMouse(); // Ważne: pozwala nie zgubić myszki przy szybkim ruchu
            }
        }

        // OBSŁUGA ZDARZEŃ MYSZKI: Przeciąganie kształtu (MouseMove)
        private void Canvas_MouseMove(object sender, MouseEventArgs e)
        {
            if (isDragging && draggedShape != null)
            {
                Point mousePos = e.GetPosition(MainCanvas);
                // Aktualizacja pozycji na Canvasie względem punktu kliknięcia
                Canvas.SetLeft(draggedShape, mousePos.X - clickPosition.X);
                Canvas.SetTop(draggedShape, mousePos.Y - clickPosition.Y);
            }
        }

        // OBSŁUGA ZDARZEŃ MYSZKI: Puszczenie przycisku (MouseLeftButtonUp)
        private void Canvas_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (isDragging && draggedShape != null)
            {
                isDragging = false;
                draggedShape.ReleaseMouseCapture();
                draggedShape = null;
            }
        }

        // Odznaczanie elementu po kliknięciu w puste tło Canvasu
        private void Canvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.OriginalSource is Canvas && selectedShape != null)
            {
                selectedShape.Stroke = Brushes.Transparent;
                selectedShape = null;
            }
        }

        // OBSŁUGA KLAWIATURY: Usuwanie elementu klawiszem 'Delete'
        private void Window_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Delete && selectedShape != null)
            {
                MainCanvas.Children.Remove(selectedShape);
                selectedShape = null;
            }
        }

        // MENU KONTEKSTOWE: Zmiana koloru
        private void ChangeColor_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuItem menuItem && menuItem.Parent is ContextMenu menu && menu.PlacementTarget is Shape shape)
            {
                shape.Fill = Brushes.Crimson;
            }
        }

        // MENU KONTEKSTOWE: Usunięcie kształtu
        private void DeleteShapeMenu_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuItem menuItem && menuItem.Parent is ContextMenu menu && menu.PlacementTarget is Shape shape)
            {
                MainCanvas.Children.Remove(shape);
                if (selectedShape == shape) selectedShape = null; // Czyszczenie zmiennej
            }
        }
    }
}