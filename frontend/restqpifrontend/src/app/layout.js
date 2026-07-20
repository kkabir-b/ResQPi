import "./globals.css";

export const metadata = {
    title: "ResQPi",
    description: "Rescue people from emergencies!",
};

export default function RootLayout({ children }) {
    return (
        <html lang="en">
            <body>
                {children}
            </body>
        </html>
    );
}