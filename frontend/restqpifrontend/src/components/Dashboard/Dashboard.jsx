"use client";

import { useEffect, useState } from "react";
import styles from "./Dashboard.module.css";
import LocationCard from "../LocationCard/LocationCard";

export default function Dashboard() {
    const [locations, setLocations] = useState([]);
    const [loading, setLoading] = useState(true);

    async function loadLocations() {
        try {
            const response = await fetch("http://localhost:8000/locations");

            if (!response.ok) {
                throw new Error("Failed to fetch locations");
            }

            const data = await response.json();

            setLocations(data);
        } catch (error) {
            console.error(error);
        } finally {
            setLoading(false);
        }
    }

    async function resetLocation(name) {
        try {
            await fetch("http://localhost:8000/reset", {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                },
                body: JSON.stringify({
                    location: name,
                }),
            });

            loadLocations();
        } catch (error) {
            console.error(error);
        }
    }

    useEffect(() => {
        loadLocations();

        const interval = setInterval(loadLocations, 2000);

        return () => clearInterval(interval);
    }, []);

    const emergencyCount = locations.filter(
        (location) => location.emergency
    ).length;

    return (
        <main className={styles.dashboard}>
            <header className={styles.header}>
                <div>
                    <h1>ResQPi dashboard</h1>
                    <p>Live monitoring of all locations</p>
                </div>

                <div className={styles.counter}>
                    <span>Active Emergencies</span>
                    <h2>{emergencyCount}</h2>
                </div>
            </header>

            {loading ? (
                <div className={styles.loading}>
                    Loading...
                </div>
            ) : (
                <div className={styles.grid}>
                    {locations.map((location) => (
                        <LocationCard
                            key={location.name}
                            location={location}
                            onReset={resetLocation}
                        />
                    ))}
                </div>
            )}
        </main>
    );
}