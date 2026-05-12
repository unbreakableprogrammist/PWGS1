# PwSG: UI + HTML

## Let's talk about UI Design (1 Pts):

Your task is to draw on paper (or equivalent on your PC) a simple 'create an account' form and describe what kind of validation each of the field requires. It should contain login, password, email address and date of birth fields and a submit button. 

Once you are finished, let's discuss your solutions with other students and teacher altogether.

## Task: Login Form with Validation (3 Pts):

Your task is to create a simple responsive login form using **only** plain HTML and CSS. The form should have the following features:
* A login field (text input for the username).
* A password field (text input for password).
* An email field (text input for email address).
* A date of birth field (input for date of birth).
* Basic validation for the email and date fields.
* Responsive design.

###  Requirements: Functionality (1 Pts):

| Requirement | Description | Points |
|------------|-------------|--------|
| 1. Username & Password fields | - Include two **required** fields: `username` and `password`. | 0.25 Pts. |
| 2. Email & Date-of-Birth fields | - Include two **required** fields: `email` and `date-of-birth`. | 0.25 Pts. |
| 3. Submit Button | - Include `submit` button that spans across two columns | 0.25 Pts. | 
| 4. Grouping and layout | - Pair fields logically: Username + Password, Email + Date-of-Birth. <br> - Ensure correct HTML structure with labels and inputs inside form groups. | 0.25 Pts. |

###  Requirements: Visuals (2 Pts)

| Requirement | Description | Points |
|------------|-------------|--------|
| 1. Overall layout and alignment | - Center the form vertically and horizontally in the viewport. <br> - Apply a **card-style design** using background, padding, border-radius, and subtle shadow. | 1.00 Pts. |
| 2. Form element styling | - Style all inputs for consistency: padding, borders, and font size. <br> - Add focus styles (e.g., border color change). <br> - Style the submit button with color and hover effect. | 0.50 Pts. |
| 3. Responsive visual adaptation | - Use layout techniques to make the form fields appear in **two columns** on large screens and **one column** on small screens (< 900px). | 0.25 Pts. |
| 4. Validation for Email & Date-Of-Brith fields | - Create basic validation for `email` and `date-of-birth` fields. <br/> - Email should contain at least `@`, whereas Date-Of-Birth should be in the past. | 0.25 Pts. |

For styling please provide following features:
* Use serif-Arial font, change its color to #f4f4f4 and alignment to center,
* Use a white background for card/form with padding of 10px, corners should have radius of 8px, black 10% opacity shadow with 10px width,
* Labels are using bold font for their text,
* Use corner radius of 4px and a solid 1px border for inputs,
* Change border color of inputs once they're in focus,
* Use #007BFF background color for button, corner radius of 4px, no borders, 16px font size, change cursor to pointer,
* Button should always occupy whole width of a card/form and change its color slightly upon hover,
* The whole form has to be responsive, meaning that on below 900px screens two columns for input should collapse into a single column.